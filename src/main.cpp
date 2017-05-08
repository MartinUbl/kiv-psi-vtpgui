#include "general.h"

#include "NetworkDefs.h"
#include "VTPDefs.h"

#include "Network.h"
#include "FrameHandlers.h"

#include <QApplication>
#include "gui/mainwindow.h"

int main(int argc, char **argv)
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    return a.exec();

    /*std::list<NetworkDeviceListEntry> devList;

    if (sNetwork->GetDeviceList(devList) != 0)
    {
        std::cerr << "Unable to retrieve device list, exiting" << std::endl;
        return 1;
    }

    int i = 0;
    for (NetworkDeviceListEntry& dev : devList)
    {
        std::cout << (++i) << ") " << dev.pcapName.c_str() << std::endl;
        std::cout << "    " << dev.pcapDesc.c_str() << std::endl;
        for (std::string& addr : dev.addresses)
            std::cout << "    " << addr.c_str() << std::endl;
    }

    int inum;
    std::cout << "Enter the interface number (1-" << i << "): ";
    std::cin >> inum;

    if (inum <= 0 || inum > i)
    {
        std::cerr << "Interface number out of range, exiting" << std::endl;
        return 2;
    }

    std::list<NetworkDeviceListEntry>::iterator devItr = devList.begin();
    std::advance(devItr, inum - 1);

    if (sNetwork->SelectDevice((*devItr).pcapName.c_str()) != 0)
    {
        std::cerr << "Unable to open interface, exiting" << std::endl;
        return 2;
    }*/

    sNetwork->Run();

    return 0;
}
