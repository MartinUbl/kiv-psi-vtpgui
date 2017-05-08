/********************************
 * KIV/PSI: VTP node            *
 * Semestral work               *
 *                              *
 * Author: Martin UBL           *
 *         A16N0026P            *
 *         ublm@students.zcu.cz *
 ********************************/

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

    selectedInterface = "";

    // retrieve device list
    std::list<NetworkDeviceListEntry> devList;
    if (sNetwork->GetDeviceList(devList) != 0)
    {
        ui.countLabel->setText("Unable to retrieve interface list");
        std::cerr << "Unable to retrieve interface list" << std::endl;
    }
    else
    {
        // build list model
        QStandardItemModel* model = new QStandardItemModel(0, 3, this);

        model->setHorizontalHeaderItem(0, new QStandardItem("Addresses"));
        model->setHorizontalHeaderItem(1, new QStandardItem("Name"));
        model->setHorizontalHeaderItem(2, new QStandardItem("Description"));

        // add devices to list
        for (NetworkDeviceListEntry& dev : devList)
        {
            QList<QStandardItem*> wl;

            if (dev.addresses.size() == 0)
            {
                wl.push_back(new QStandardItem("-"));
            }
            else // parse addresses to one string, delimited by commas
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

            // append name and description
            wl.push_back(new QStandardItem(dev.pcapName.c_str()));
            m_interfaceNames.push_back(dev.pcapName);
            if (dev.pcapDesc.length() > 0)
                wl.push_back(new QStandardItem(dev.pcapDesc.c_str()));
            else
                wl.push_back(new QStandardItem("-"));

            model->appendRow(wl);
        }

        // set model, update labels
        ui.devlistView->setModel(model);
        ui.countLabel->setText(QString("Found ") + std::to_string(devList.size()).c_str() + QString(" interface(s)"));
    }

    // connect button events
    connect(ui.OKButton, SIGNAL(clicked()), this, SLOT(OKButton_clicked()));
    connect(ui.CancelButton, SIGNAL(clicked()), this, SLOT(CancelButton_clicked()));
}

void devselectdialog::OKButton_clicked()
{
    QModelIndex index = ui.devlistView->currentIndex();

    // domain must be entered
    if (ui.domainLineEdit->text().isEmpty())
    {
        QMessageBox::information(this, "No domain selected", "You have to choose VTP domain", QMessageBox::StandardButton::Ok);
        return;
    }

    // limit domain length
    if (ui.domainLineEdit->text().length() >= MAX_VTP_DOMAIN_LENGTH)
    {
        QMessageBox::information(this, "Error", "VTP domain name is too long! Choose name shorter than 32 characters", QMessageBox::StandardButton::Ok);
        return;
    }

    // store to app globals
    sAppGlobals->g_VTPDomain = ui.domainLineEdit->text().toStdString();
    sAppGlobals->g_VTPPassword = ui.passwordLineEdit->text().toStdString();

    // check index validity
    if (!index.isValid() || (int)m_interfaceNames.size() <= index.row())
        QMessageBox::information(this, "No interface selected", "You have to select interface from interface list", QMessageBox::StandardButton::Ok);
    else
    {
        //std::cout << "INT: " << ui.devlistView->model()->data(index) << std::endl;
        selectedInterface = m_interfaceNames[index.row()];
        close();
    }
}

void devselectdialog::CancelButton_clicked()
{
    selectedInterface = "";
    close();
}

devselectdialog::~devselectdialog()
{
}
