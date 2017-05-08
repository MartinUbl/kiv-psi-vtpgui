/********************************
 * KIV/PSI: VTP node            *
 * Semestral work               *
 *                              *
 * Author: Martin UBL           *
 *         A16N0026P            *
 *         ublm@students.zcu.cz *
 ********************************/

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStandardItemModel>
#include <QCloseEvent>

#include "ui_mainwindow.h"
#include "../GUIDefs.h"
#include "../VTPDefs.h"

class MainWindow : public QMainWindow
{
        Q_OBJECT

    public:
        explicit MainWindow(QWidget *parent = 0);
        ~MainWindow();

        // adds traffic entry (frame transmitted) with appropriate dump
        void AddTrafficEntry(uint64_t timestamp, bool outgoing, uint8_t code, std::string codeDescription, FrameDumpContents* dumpProps);
        // clears all VLANs
        void ClearVLANView();
        // adds VLAN to vector and to model
        void AddVLAN(uint16_t id, std::string name, VLANRecord* vlan);

    protected:
        // initializes all table views (their columns, etc.)
        void InitTableViews();

    private:
        Ui::MainWindow ui;

        // traffic model
        QStandardItemModel *m_trafficModel;
        // traffic contents model
        QStandardItemModel *m_trafficDumpModel;
        // all dumps stored
        std::vector<FrameDumpContents*> m_trafficDumps;

        // all vlans model
        QStandardItemModel *m_vlanModel;
        // vlan properties model
        QStandardItemModel *m_vlanPropModel;
        // all stored vlans
        std::vector<VLANRecord*> m_vlans;

        // selected item from traffic table
        int selectedTrafficItem;

        // application close event
        void closeEvent(QCloseEvent *event);

    private slots:
        void TrafficItem_Selected(const QModelIndex &);
        void TrafficDumpItem_Selected(const QModelIndex &);
        void VLANItem_Selected(const QModelIndex &);
        void AdvertReqButton_Clicked();
        void ClearAllButton_Clicked();
        void SendVlansButton_Clicked();
        void SendSummaryButton_Clicked();

        void ActionAbout_Clicked();
        void ActionExit_Clicked();
};

#endif // MAINWINDOW_H
