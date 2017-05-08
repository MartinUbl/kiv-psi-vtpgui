/********************************
 * KIV/PSI: VTP node            *
 * Semestral work               *
 *                              *
 * Author: Martin UBL           *
 *         A16N0026P            *
 *         ublm@students.zcu.cz *
 ********************************/

#ifndef GLOBALS_H
#define GLOBALS_H

#include <QWidget>
#include "gui/mainwindow.h"

#include "Singleton.h"

/*
 * Application-global data storage
 */
class VTPAppGlobals : public CSingleton<VTPAppGlobals>
{
    public:
        // main window reference
        MainWindow* g_MainWindow;

        // MAC address used
        uint8_t g_MACAddress[MAC_ADDR_LENGTH];

        // VTP domain used
        std::string g_VTPDomain;
        // VTP secret used
        std::string g_VTPPassword;
};

#define sAppGlobals VTPAppGlobals::getInstance()

#endif
