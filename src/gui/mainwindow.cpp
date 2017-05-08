#include "../general.h"

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "devselectdialog.h"

#include <QMessageBox>
#include <QPixmap>

#include "../Network.h"
#include "../FrameHandlers.h"
#include "../Globals.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui()
{
    ui.setupUi(this);

    sAppGlobals->g_MainWindow = this;

    int res = -1;
    std::string errbuf;
    do
    {
        devselectdialog diag(this);
        diag.exec();

        res = diag.selectedInterface;

        if (res < 0)
        {
            int modres = QMessageBox::critical(this, "Error", "You must select the interface to be used! Return to interface selection?", QMessageBox::StandardButton::Yes, QMessageBox::StandardButton::No);

            if (modres == QMessageBox::StandardButton::No)
            {
                // TODO: do not allow to start application
                break;
            }
        }

        std::list<NetworkDeviceListEntry> devList;
        sNetwork->GetDeviceList(devList);

        if (res >= 0 && res < (int)devList.size())
        {
            auto itr = devList.begin();
            std::advance(itr, res);

            if (sNetwork->SelectDevice((*itr).pcapName.c_str(), errbuf) < 0)
            {
                QMessageBox::critical(this, "Error", ("Could not open selected device: " + errbuf).c_str());
                res = -1;
            }
        }

    } while (res < 0);

    InitTableViews();

    if (res < 0)
        return;

    sNetwork->Run();

    connect(ui.trafficTableView, SIGNAL(clicked(const QModelIndex &)), this, SLOT(TrafficItem_Selected(const QModelIndex &)));
    connect(ui.frameDumpTableView, SIGNAL(clicked(const QModelIndex &)), this, SLOT(TrafficDumpItem_Selected(const QModelIndex &)));
    connect(ui.vlanTableView, SIGNAL(clicked(const QModelIndex &)), this, SLOT(VLANItem_Selected(const QModelIndex &)));
    connect(ui.advertRequestSendButton, SIGNAL(clicked()), this, SLOT(AdvertReqButton_Clicked()));
    connect(ui.clearAllButton, SIGNAL(clicked()), this, SLOT(ClearAllButton_Clicked()));
    connect(ui.actionAboutApp, SIGNAL(triggered()), this, SLOT(ActionAbout_Clicked()));
    connect(ui.actionTerminateApp, SIGNAL(triggered()), this, SLOT(ActionExit_Clicked()));
}

void MainWindow::InitTableViews()
{
    m_trafficModel = new QStandardItemModel(0, 4, this);
    m_trafficModel->setHorizontalHeaderItem(0, new QStandardItem(QString("Timestamp")));
    m_trafficModel->setHorizontalHeaderItem(1, new QStandardItem(QString("Direction")));
    m_trafficModel->setHorizontalHeaderItem(2, new QStandardItem(QString("VTP code")));
    m_trafficModel->setHorizontalHeaderItem(3, new QStandardItem(QString("Description")));

    ui.trafficTableView->setModel(m_trafficModel);
    ui.trafficTableView->horizontalHeader()->setStretchLastSection(true);

    m_trafficDumpModel = new QStandardItemModel(0, 2, this);
    m_trafficDumpModel->setHorizontalHeaderItem(0, new QStandardItem(QString("Key")));
    m_trafficDumpModel->setHorizontalHeaderItem(1, new QStandardItem(QString("Value")));
    ui.frameDumpTableView->setModel(m_trafficDumpModel);
    ui.frameDumpTableView->horizontalHeader()->setStretchLastSection(true);

    QPalette p = ui.frameContents->palette();
    p.setColor(QPalette::Highlight, Qt::GlobalColor::darkBlue);
    p.setColor(QPalette::HighlightedText, Qt::GlobalColor::white);
    ui.frameContents->setPalette(p);


    m_vlanModel = new QStandardItemModel(0, 2, this);
    m_vlanModel->setHorizontalHeaderItem(0, new QStandardItem(QString("ID")));
    m_vlanModel->setHorizontalHeaderItem(1, new QStandardItem(QString("Name")));
    ui.vlanTableView->setModel(m_vlanModel);
    ui.vlanTableView->horizontalHeader()->setStretchLastSection(true);


    m_vlanPropModel = new QStandardItemModel(0, 2, this);
    m_vlanPropModel->setHorizontalHeaderItem(0, new QStandardItem(QString("Key")));
    m_vlanPropModel->setHorizontalHeaderItem(1, new QStandardItem(QString("Value")));
    ui.vlanPropertiesView->setModel(m_vlanPropModel);
    ui.vlanPropertiesView->horizontalHeader()->setStretchLastSection(true);
}

void MainWindow::AddTrafficEntry(uint64_t timestamp, bool outgoing, uint8_t code, std::string codeDescription, FrameDumpContents* dumpProps)
{
    QList<QStandardItem*> wl;

    wl.push_back(new QStandardItem(QString::number(timestamp)));

    QImage image(outgoing ? ":/icons/res/upload.png" : ":/icons/res/download.png", nullptr);
    QStandardItem *item = new QStandardItem();
    item->setData(QVariant(QPixmap::fromImage(image)), Qt::DecorationRole);
    wl.push_back(item);

    wl.push_back(new QStandardItem(QString::number(code)));
    wl.push_back(new QStandardItem(QString(codeDescription.c_str())));

    m_trafficModel->appendRow(wl);

    m_trafficDumps.push_back(dumpProps);
}

void MainWindow::ClearVLANView()
{
    m_vlanPropModel->clear();
    m_vlanModel->clear();

    for (size_t i = 0; i < m_vlans.size(); i++)
        delete m_vlans[i];
    m_vlans.clear();

    m_vlanModel->setHorizontalHeaderItem(0, new QStandardItem(QString("ID")));
    m_vlanModel->setHorizontalHeaderItem(1, new QStandardItem(QString("Name")));
    ui.vlanTableView->horizontalHeader()->setStretchLastSection(true);
}

void MainWindow::AddVLAN(uint16_t id, std::string name, VLANRecord* vlan)
{
    QList<QStandardItem*> wl;

    wl.push_back(new QStandardItem(QString::number(id)));
    wl.push_back(new QStandardItem(QString(name.c_str())));

    VLANRecord* cpy = new VLANRecord(*vlan);
    m_vlans.push_back(cpy);

    m_vlanModel->appendRow(wl);
    ui.vlanTableView->horizontalHeader()->setStretchLastSection(true);
}

void MainWindow::TrafficItem_Selected(const QModelIndex &index)
{
    selectedTrafficItem = index.row();

    ui.frameContents->setText(m_trafficDumps[index.row()]->contents.c_str());

    m_trafficDumpModel->clear();
    m_trafficDumpModel->setHorizontalHeaderItem(0, new QStandardItem(QString("Key")));
    m_trafficDumpModel->setHorizontalHeaderItem(1, new QStandardItem(QString("Value")));
    ui.frameDumpTableView->horizontalHeader()->setStretchLastSection(true);

    FrameDumpContents* dump = m_trafficDumps[index.row()];

    for (size_t i = 0; i < dump->tableRows.size(); i++)
    {
        QList<QStandardItem*> wl;

        wl.push_back(new QStandardItem(dump->tableRows[i].first.c_str()));
        wl.push_back(new QStandardItem(dump->tableRows[i].second.c_str()));

        m_trafficDumpModel->appendRow(wl);
    }
}

void MainWindow::TrafficDumpItem_Selected(const QModelIndex &index)
{
    FrameDumpContents* dump = m_trafficDumps[selectedTrafficItem];
    auto spair = dump->dumpStartEnds[index.row()];

    QTextCursor c = ui.frameContents->textCursor();
    c.setPosition(spair.first * 3);
    c.setPosition(spair.second * 3 - 1, QTextCursor::KeepAnchor);
    ui.frameContents->setTextCursor(c);
}

void MainWindow::VLANItem_Selected(const QModelIndex &index)
{
    m_vlanPropModel->clear();

    VLANRecord* rec = m_vlans[index.row()];

#define ADD_PROP_ROW(key,value) { QList<QStandardItem*> wl; wl.push_back(new QStandardItem(key)); wl.push_back(new QStandardItem(value)); m_vlanPropModel->appendRow(wl); }

    ADD_PROP_ROW("ID", std::to_string(rec->id).c_str());
    ADD_PROP_ROW("Name", rec->name.c_str());
    ADD_PROP_ROW("Type", std::to_string(rec->type).c_str());
    ADD_PROP_ROW("Status", std::to_string(rec->status).c_str());
    ADD_PROP_ROW("MTU", std::to_string(rec->mtu).c_str());
    ADD_PROP_ROW("802.10 Index", std::to_string(rec->index80210).c_str());

    for (auto fp : rec->features)
        ADD_PROP_ROW(VLANFeatureNames[fp.first], std::to_string(fp.second).c_str());

    m_vlanPropModel->setHorizontalHeaderItem(0, new QStandardItem(QString("Key")));
    m_vlanPropModel->setHorizontalHeaderItem(1, new QStandardItem(QString("Value")));
    ui.vlanPropertiesView->horizontalHeader()->setStretchLastSection(true);
}

void MainWindow::AdvertReqButton_Clicked()
{
    sFrameHandler->SendAdvertRequest();
}

void MainWindow::ClearAllButton_Clicked()
{
    ClearVLANView();

    ui.frameContents->setText("");

    m_trafficDumpModel->clear();
    m_trafficDumpModel->setHorizontalHeaderItem(0, new QStandardItem(QString("Key")));
    m_trafficDumpModel->setHorizontalHeaderItem(1, new QStandardItem(QString("Value")));
    ui.frameDumpTableView->horizontalHeader()->setStretchLastSection(true);

    m_vlanPropModel->clear();
    m_vlanPropModel->setHorizontalHeaderItem(0, new QStandardItem(QString("Key")));
    m_vlanPropModel->setHorizontalHeaderItem(1, new QStandardItem(QString("Value")));
    ui.vlanPropertiesView->horizontalHeader()->setStretchLastSection(true);

    for (size_t i = 0; i < m_trafficDumps.size(); i++)
        delete m_trafficDumps[i];
    m_trafficDumps.clear();

    m_trafficModel->clear();
    m_trafficModel->setHorizontalHeaderItem(0, new QStandardItem(QString("Timestamp")));
    m_trafficModel->setHorizontalHeaderItem(1, new QStandardItem(QString("Direction")));
    m_trafficModel->setHorizontalHeaderItem(2, new QStandardItem(QString("VTP code")));
    m_trafficModel->setHorizontalHeaderItem(3, new QStandardItem(QString("Description")));
    ui.trafficTableView->horizontalHeader()->setStretchLastSection(true);
}

void MainWindow::ActionAbout_Clicked()
{
    QMessageBox about;

    about.setWindowTitle("About");
    about.setIconPixmap(QPixmap(":/icons/res/bigicon.png"));
    about.setInformativeText("<b>KIV/PSI: VTP</b><br/>Semestral work for \"Computer networking\" subject<br/><br/>\
        University of West Bohemia,<br/>Faculty of Applied Sciences,<br/>Department of Computer Science and Engineering<br/><br/>\
        <u>Author</u>: Martin Ubl (A16N0026P)<br/><a href=\"mailto:ublm@students.zcu.cz\">ublm@students.zcu.cz</a><br/>Copyright (c) 2017");
    about.setStandardButtons(QMessageBox::Ok);
    about.setDefaultButton(QMessageBox::Ok);
    about.show();
    about.exec();
}

void MainWindow::ActionExit_Clicked()
{
    QApplication::exit(0);
}

MainWindow::~MainWindow()
{
    //
}
