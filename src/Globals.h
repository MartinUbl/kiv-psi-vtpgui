#ifndef GLOBALS_H
#define GLOBALS_H

#include <QWidget>
#include "gui/mainwindow.h"

#include "Singleton.h"

class VTPAppGlobals : public CSingleton<VTPAppGlobals>
{
    public:
        MainWindow* g_MainWindow;

        uint8_t g_MACAddress[MAC_ADDR_LENGTH];
};

#define sAppGlobals VTPAppGlobals::getInstance()

#endif
