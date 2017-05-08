#ifndef NETWORK_H
#define NETWORK_H

#include "Singleton.h"
#include "NetworkDefs.h"

#include <pcap.h>
#include <list>

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
        pcap_t* m_dev;
        SendingTemplate m_sendingTemplate;

        uint8_t* m_header;
        uint16_t m_headerLenght;

        uint16_t* m_headerDataLengthField;

    protected:
        void _Run();

    public:
        NetworkService();

        int GetDeviceList(std::list<NetworkDeviceListEntry>& target);

        int SelectDevice(const char* pcapName, std::string& err);

        void Run();

        bool HasSendingTemplate();
        void SetSendingTemplate(SendingTemplate templType, uint16_t vlanId = 0);

        void SendUsingTemplate(uint8_t* data, uint16_t dataLength);
};

#define sNetwork NetworkService::getInstance()

#endif
