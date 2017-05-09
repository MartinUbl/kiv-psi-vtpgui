/********************************
 * KIV/PSI: VTP node            *
 * Semestral work               *
 *                              *
 * Author: Martin UBL           *
 *         A16N0026P            *
 *         ublm@students.zcu.cz *
 ********************************/

#include "general.h"
#include "Network.h"

#include "FrameHandlers.h"
#include "Globals.h"

// "translations" of VLAN features for GUI output
const char* VLANFeatureNames[MAX_VLAN_FEATURE] = {
    "Feature - none",
    "Ring No.",
    "Bridge No.",
    "STP",
    "Parent",
    "Trans1",
    "Trans2",
    "BridgeMode",
    "ARE Hops",
    "STE Hops",
    "BackupCRF"
};

NetworkService::NetworkService()
{
    m_headerDataLengthField = nullptr;
    m_networkThread = nullptr;
    m_dev = nullptr;
    m_header = nullptr;

    // default to 802.3 Ethernet
    m_sendingTemplate = SendingTemplate::NET_SEND_TEMPLATE_NONE;
    SetSendingTemplate(SendingTemplate::NET_SEND_TEMPLATE_8023, 0);
    m_sendingTemplate = SendingTemplate::NET_SEND_TEMPLATE_NONE;
}

int NetworkService::GetDeviceList(std::list<NetworkDeviceListEntry>& target)
{
    pcap_if_t *alldevs, *d;
    char errbuf[PCAP_ERRBUF_SIZE];
    char dsaddr[32];
    pcap_addr* ad;

    // get all devices
#ifdef _WIN32
    if (pcap_findalldevs_ex(PCAP_SRC_IF_STRING, NULL, &alldevs, errbuf) == -1)
#else
    if (pcap_findalldevs(&alldevs, errbuf) == -1)
#endif
    {
        std::cerr << "Error in pcap_findalldevs_ex: " << errbuf << std::endl;
        return -1;
    }

    target.clear();

    // parse devices to list
    for (d = alldevs; d; d = d->next)
    {
        NetworkDeviceListEntry dev;

        dev.pcapName = d->name;
        if (d->description)
            dev.pcapDesc = d->description;

        for (ad = d->addresses; ad; ad = ad->next)
        {
            // for now, list only IPv4 addresses
            if (ad->addr->sa_family != AF_INET)
                continue;

            inet_ntop(ad->addr->sa_family, &((sockaddr_in*)ad->addr)->sin_addr, dsaddr, 32);
            dev.addresses.push_back(dsaddr);
        }

        target.push_back(dev);
    }

    pcap_freealldevs(alldevs);

    if (target.size() == 0)
    {
        std::cerr << "No interfaces found!" << std::endl;
        return -1;
    }

    return 0;
}


#ifdef _WIN32

#include <Windows.h>
#include <Iphlpapi.h>
#include <Assert.h>
#pragma comment(lib, "iphlpapi.lib")

#endif

int getMAC(std::string adapterName, uint8_t* buffer)
{
#ifdef _WIN32
    PIP_ADAPTER_INFO AdapterInfo;
    DWORD dwBufLen = sizeof(AdapterInfo);
    char *mac_addr = (char*)malloc(17);

    AdapterInfo = (IP_ADAPTER_INFO *)malloc(sizeof(IP_ADAPTER_INFO));
    if (AdapterInfo == NULL)
    {
        std::cerr << "Error allocating memory needed to call GetAdaptersinfo" << std::endl;
        return -1;
    }

    if (GetAdaptersInfo(AdapterInfo, &dwBufLen) == ERROR_BUFFER_OVERFLOW)
    {
        AdapterInfo = (IP_ADAPTER_INFO *)malloc(dwBufLen);
        if (AdapterInfo == NULL)
        {
            std::cerr << "Error allocating memory needed to call GetAdaptersinfo" << std::endl;
            return -1;
        }
    }

    if (GetAdaptersInfo(AdapterInfo, &dwBufLen) == NO_ERROR)
    {
        for (PIP_ADAPTER_INFO pAdapterInfo = AdapterInfo; pAdapterInfo; pAdapterInfo = pAdapterInfo->Next)
        {
            if (pAdapterInfo->AdapterName && strlen(pAdapterInfo->AdapterName) > 2)
            {
                std::string adId(pAdapterInfo->AdapterName);
                if (adapterName.find(adId) == std::string::npos)
                    continue;
            }
            else
                continue;

            memcpy(buffer, pAdapterInfo->Address, MAC_ADDR_LENGTH);

            free(AdapterInfo);

            return 0;
        }
    }

    free(AdapterInfo);

    return -1;
#else
    // TODO: linux part

    return -1;
#endif
}

int NetworkService::SelectDevice(const char* pcapName, std::string& err)
{
    char errbuf[PCAP_ERRBUF_SIZE];

    // open device if available
#ifdef _WIN32
    m_dev = pcap_open(pcapName, 100, PCAP_OPENFLAG_PROMISCUOUS, 20, nullptr, errbuf);
#else
    m_dev = pcap_open_live(pcapName, 100, 1, 20, errbuf);
#endif

    if (!m_dev)
    {
        std::cerr << "Error opening adapter: " << errbuf << std::endl;
        err = errbuf;
        return -1;
    }

    for (size_t i = 0; i < MAC_ADDR_LENGTH; i++)
        sAppGlobals->g_MACAddress[i] = (uint8_t)(0xF0 + i);
    memcpy(m_sourceMacField, sAppGlobals->g_MACAddress, MAC_ADDR_LENGTH);

    if (getMAC(pcapName, m_sourceMacField) < 0)
        std::cerr << "Could not find adapter MAC address, using mocked" << std::endl;
    else
        memcpy(sAppGlobals->g_MACAddress, m_sourceMacField, MAC_ADDR_LENGTH);

    return 0;
}

void NetworkService::_Run()
{
    int res;
    pcap_pkthdr *header;
    const uint8_t *pkt_data;

    m_running = true;

    // packet dump loop
    while (m_running && (res = pcap_next_ex(m_dev, &header, &pkt_data)) >= 0)
    {
        // timeout
        if (res <= 0)
            continue;

        sFrameHandler->HandleIncoming(header, pkt_data);
    }
}

void NetworkService::Run()
{
    m_networkThread = new std::thread(&NetworkService::_Run, this);
}

void NetworkService::Finalize()
{
    m_running = false;
    if (m_dev)
        pcap_close(m_dev);

    if (m_networkThread)
        m_networkThread->join();
}

bool NetworkService::HasSendingTemplate()
{
    return (m_sendingTemplate != SendingTemplate::NET_SEND_TEMPLATE_NONE);
}

void NetworkService::SetSendingTemplate(SendingTemplate templType, uint16_t vlanId)
{
    if (HasSendingTemplate())
        delete m_header;

    LLCHeader* llc;
    SNAPHeader* snap;

    switch (templType)
    {
        case SendingTemplate::NET_SEND_TEMPLATE_8023:
        {
            m_headerLenght = sizeof(EthernetHeader) + sizeof(LLCHeader) + sizeof(SNAPHeader);
            m_header = new uint8_t[m_headerLenght];
            EthernetHeader* eh = (EthernetHeader*)m_header;
            llc = (LLCHeader*)(m_header + sizeof(EthernetHeader));
            snap = (SNAPHeader*)(m_header + sizeof(EthernetHeader) + sizeof(LLCHeader));

            memcpy(eh->src_mac, sAppGlobals->g_MACAddress, MAC_ADDR_LENGTH);
            memcpy(eh->dest_mac, VTP_Dest_MAC, MAC_ADDR_LENGTH);
            eh->frame_length = 0;
            m_headerDataLengthField = &eh->frame_length;
            m_sourceMacField = eh->src_mac;
            break;
        }
        case SendingTemplate::NET_SEND_TEMPLATE_DOT1Q:
        {
            m_headerLenght = sizeof(Dot1QHeader) + sizeof(LLCHeader) + sizeof(SNAPHeader);
            m_header = new uint8_t[m_headerLenght];
            Dot1QHeader* eh = (Dot1QHeader*)m_header;
            llc = (LLCHeader*)(m_header + sizeof(Dot1QHeader));
            snap = (SNAPHeader*)(m_header + sizeof(Dot1QHeader) + sizeof(LLCHeader));

            memcpy(eh->src_mac, sAppGlobals->g_MACAddress, MAC_ADDR_LENGTH);
            memcpy(eh->dest_mac, VTP_Dest_MAC, MAC_ADDR_LENGTH);
            eh->tpid = DOT1Q_VTP_TPID;
            eh->tci = ((DOT1Q_VTP_PCP & 0x0007) << 13) | ((DOT1Q_VTP_DEI & 0x0001) << 12) | (vlanId & 0x0FFF);
            eh->frame_length = 0;
            m_headerDataLengthField = &eh->frame_length;
            m_sourceMacField = eh->src_mac;
            break;
        }
        default:
            return;
    }

    llc->dsap = SNAP_DSAP_IDENTIFIER;
    llc->ssap = SNAP_SSAP_IDENTIFIER;
    llc->control = SNAP_CONTROL_VALUE;

    snap->protocol_id = htons(VTP_IDENTIFIER);
    memcpy(snap->oui, OUI_Cisco, SNAP_OUI_LEN);

    m_sendingTemplate = templType;
}

void NetworkService::SendUsingTemplate(uint8_t* data, uint16_t dataLength)
{
    size_t totalLen = m_headerLenght + dataLength;

    uint8_t *assembled = new uint8_t[totalLen];

    *m_headerDataLengthField = htons((uint16_t)(totalLen - sizeof(EthernetHeader)));

    memcpy(assembled, m_header, m_headerLenght);
    memcpy(assembled + m_headerLenght, data, dataLength);

    if (pcap_sendpacket(m_dev, assembled, totalLen) != 0)
        std::cerr << "Unable to send frame via pcap device" << std::endl;

    delete[] assembled;
}
