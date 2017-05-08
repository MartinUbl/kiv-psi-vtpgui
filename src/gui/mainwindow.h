#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStandardItemModel>

#include "ui_mainwindow.h"
#include "../GUIDefs.h"
#include "../VTPDefs.h"

class MainWindow : public QMainWindow
{
        Q_OBJECT

    public:
        explicit MainWindow(QWidget *parent = 0);
        ~MainWindow();

        void AddTrafficEntry(uint64_t timestamp, bool outgoing, uint8_t code, std::string codeDescription, FrameDumpContents* dumpProps);

        void ClearVLANView();
        void AddVLAN(uint16_t id, std::string name, VLANRecord* vlan);

    protected:
        void InitTableViews();

    private:
        Ui::MainWindow ui;

        QStandardItemModel *m_trafficModel;
        QStandardItemModel *m_trafficDumpModel;
        std::vector<FrameDumpContents*> m_trafficDumps;

        QStandardItemModel *m_vlanModel;
        QStandardItemModel *m_vlanPropModel;
        std::vector<VLANRecord*> m_vlans;

        int selectedTrafficItem;

    private slots:
        void TrafficItem_Selected(const QModelIndex &);
        void TrafficDumpItem_Selected(const QModelIndex &);
        void VLANItem_Selected(const QModelIndex &);
        void AdvertReqButton_Clicked();
        void ClearAllButton_Clicked();

        void ActionAbout_Clicked();
        void ActionExit_Clicked();
};

#endif // MAINWINDOW_H
