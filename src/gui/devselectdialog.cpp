#include "../general.h"
#include "devselectdialog.h"

#include <QStringListModel>
#include <QMessageBox>
#include <QStandardItemModel>

#include "../Network.h"
#include "../Globals.h"

devselectdialog::devselectdialog(QWidget *parent) : QDialog(parent)
{
    ui.setupUi(this);

    selectedInterface = -1;

    std::list<NetworkDeviceListEntry> devList;

    if (sNetwork->GetDeviceList(devList) != 0)
    {
        ui.countLabel->setText("Unable to retrieve interface list");
        std::cerr << "Unable to retrieve interface list" << std::endl;
    }
    else
    {
        QStandardItemModel* model = new QStandardItemModel(0, 3, this);

        model->setHorizontalHeaderItem(0, new QStandardItem("Addresses"));
        model->setHorizontalHeaderItem(1, new QStandardItem("Name"));
        model->setHorizontalHeaderItem(2, new QStandardItem("Description"));

        for (NetworkDeviceListEntry& dev : devList)
        {
            QList<QStandardItem*> wl;

            if (dev.addresses.size() == 0)
            {
                wl.push_back(new QStandardItem("-"));
            }
            else
            {
                std::string item = "";

                bool frst = true;
                for (std::string& addr : dev.addresses)
                {
                    if (frst)
                        frst = false;
                    else
                        item += ", ";

                    item += addr;
                }

                wl.push_back(new QStandardItem(item.c_str()));
            }

            wl.push_back(new QStandardItem(dev.pcapName.c_str()));
            if (dev.pcapDesc.length() > 0)
                wl.push_back(new QStandardItem(dev.pcapDesc.c_str()));
            else
                wl.push_back(new QStandardItem("-"));

            model->appendRow(wl);
        }

        ui.devlistView->setModel(model);

        ui.countLabel->setText(QString("Found ") + std::to_string(devList.size()).c_str() + QString(" interface(s)"));
    }

    connect(ui.OKButton, SIGNAL(clicked()), this, SLOT(OKButton_clicked()));
    connect(ui.CancelButton, SIGNAL(clicked()), this, SLOT(CancelButton_clicked()));
}

void devselectdialog::OKButton_clicked()
{
    QModelIndex index = ui.devlistView->currentIndex();

    if (ui.domainLineEdit->text().isEmpty())
    {
        QMessageBox::information(this, "No domain selected", "You have to choose VTP domain", QMessageBox::StandardButton::Ok);
        return;
    }

    sAppGlobals->g_VTPDomain = ui.domainLineEdit->text().toStdString();
    sAppGlobals->g_VTPPassword = ui.passwordLineEdit->text().toStdString();

    if (!index.isValid())
        QMessageBox::information(this, "No interface selected", "You have to select interface from interface list", QMessageBox::StandardButton::Ok);
    else
    {
        selectedInterface = index.row();
        close();
    }
}

void devselectdialog::CancelButton_clicked()
{
    selectedInterface = -1;
    close();
}

devselectdialog::~devselectdialog()
{
}
