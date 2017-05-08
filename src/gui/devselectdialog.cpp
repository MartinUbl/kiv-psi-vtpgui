#include "../general.h"
#include "devselectdialog.h"

#include <QStringListModel>
#include <QMessageBox>

#include "../Network.h"

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
        QStringList sl;
        sl.clear();

        for (NetworkDeviceListEntry& dev : devList)
        {
            std::string item = dev.pcapDesc + " (";

            bool frst = true;
            for (std::string& addr : dev.addresses)
            {
                if (frst)
                    frst = false;
                else
                    item += ", ";

                item += addr;
            }
            item += "), "+dev.pcapName;

            sl.push_back(item.c_str());
        }

        QStringListModel* model = new QStringListModel();
        model->setStringList(sl);

        ui.devlistView->setModel(model);

        ui.countLabel->setText(QString("Found ") + std::to_string(devList.size()).c_str() + QString(" interface(s)"));
    }

    connect(ui.OKButton, SIGNAL(clicked()), this, SLOT(OKButton_clicked()));
    connect(ui.CancelButton, SIGNAL(clicked()), this, SLOT(CancelButton_clicked()));
}

void devselectdialog::OKButton_clicked()
{
    QModelIndex index = ui.devlistView->currentIndex();

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
