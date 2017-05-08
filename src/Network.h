/********************************
 * KIV/PSI: VTP node            *
 * Semestral work               *
 *                              *
 * Author: Martin UBL           *
 *         A16N0026P            *
 *         ublm@students.zcu.cz *
 ********************************/

#ifndef NETWORK_H
#define NETWORK_H

#include "Singleton.h"
#include "NetworkDefs.h"

#include <pcap.h>
#include <list>

#include <thread>

// enumerator of used templates for sending
enum class SendingTemplate
{
    NET_SEND_TEMPLATE_NONE      = 0,
    NET_SEND_TEMPLATE_8023      = 1,    // 802.3 Ethernet
    NET_SEND_TEMPLATE_DOT1Q     = 2     // 802.1Q encapsulation
};

/*
 * Container structure for network interface description
 */
struct NetworkDeviceListEntry
{
    // name used for PCAP opening
    std::string pcapName;
    // system interface description
    std::string pcapDesc;
    // list of IPv4 addresses of this interface
    std::list<std::string> addresses;
};

/*
 * Class maintaining network-related services, encapsulation to MAC/LLC frames, etc.
 */
class NetworkService : public CSingleton<NetworkService>
{
    private:
        // network thread used
        std::thread* m_networkThread;
        // PCAP device of opened interface
        pcap_t* m_dev;
        // sending template used in outgoing traffic
        SendingTemplate m_sendingTemplate;

        // outgoing traffic header
        uint8_t* m_header;
        // outgoing traffic header length
        uint16_t m_headerLenght;
        // pointer to header length field
        uint16_t* m_headerDataLengthField;
        // pointer to source MAC field
        uint8_t* m_sourceMacField;

        // is the thread still running?
        bool m_running;

    protected:
        // thread function
        void _Run();

    public:
        NetworkService();

        // retrieves all device list and puts it into supplied list
        int GetDeviceList(std::list<NetworkDeviceListEntry>& target);
        // selects specified device to operate on
        int SelectDevice(const char* pcapName, std::string& err);

        // run network worker
        void Run();
        // finalize network worker
        void Finalize();

        // do we have sending template set?
        bool HasSendingTemplate();
        // sets sending template to be used in outgoing traffic
        void SetSendingTemplate(SendingTemplate templType, uint16_t vlanId = 0);

        // sends frame with supplied data
        void SendUsingTemplate(uint8_t* data, uint16_t dataLength);
};

#define sNetwork NetworkService::getInstance()

#endif
