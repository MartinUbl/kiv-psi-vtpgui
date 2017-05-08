#ifndef NETWORK_H
#define NETWORK_H

#include "Singleton.h"
#include "NetworkDefs.h"

#include <pcap.h>
#include <list>

#include <thread>

enum class SendingTemplate
{
    NET_SEND_TEMPLATE_NONE      = 0,
    NET_SEND_TEMPLATE_8023      = 1,
    NET_SEND_TEMPLATE_DOT1Q     = 2
};

struct NetworkDeviceListEntry
{
    std::string pcapName;
    std::string pcapDesc;
    std::list<std::string> addresses;
};

class NetworkService : public CSingleton<NetworkService>
{
    private:
        std::thread* m_networkThread;
        pcap_t* m_dev;
        SendingTemplate m_sendingTemplate;

        uint8_t* m_header;
        uint16_t m_headerLenght;

        uint16_t* m_headerDataLengthField;
        uint8_t* m_sourceMacField;

        bool m_running;

    protected:
        void _Run();

    public:
        NetworkService();

        int GetDeviceList(std::list<NetworkDeviceListEntry>& target);

        int SelectDevice(const char* pcapName, std::string& err);

        void Run();
        void Finalize();

        bool HasSendingTemplate();
        void SetSendingTemplate(SendingTemplate templType, uint16_t vlanId = 0);

        void SendUsingTemplate(uint8_t* data, uint16_t dataLength);
};

#define sNetwork NetworkService::getInstance()

#endif
