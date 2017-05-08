#include "general.h"
#include "Network.h"

#include "FrameHandlers.h"
#include "Globals.h"

NetworkService::NetworkService()
{
    m_sendingTemplate = SendingTemplate::NET_SEND_TEMPLATE_NONE;
    SetSendingTemplate(SendingTemplate::NET_SEND_TEMPLATE_8023, 0);
    m_sendingTemplate = SendingTemplate::NET_SEND_TEMPLATE_NONE;
}

int NetworkService::GetDeviceList(std::list<NetworkDeviceListEntry>& target)
{
    pcap_if_t *alldevs, *d;
    int i = 0;
    char errbuf[PCAP_ERRBUF_SIZE];
    char dsaddr[32];
    pcap_addr* ad;

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

int NetworkService::SelectDevice(const char* pcapName, std::string& err)
{
    char errbuf[PCAP_ERRBUF_SIZE];

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

    // TODO: retrieve real MAC address

    for (size_t i = 0; i < MAC_ADDR_LENGTH; i++)
        sAppGlobals->g_MACAddress[i] = (uint8_t)(0xF0 + i);
    memcpy(m_sourceMacField, sAppGlobals->g_MACAddress, MAC_ADDR_LENGTH);

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
    pcap_close(m_dev);

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
            eh->tci = ((DOT1Q_VTP_PCP & 0x0007) << 13) | ((DOT1Q_VTP_DEI & 0x0001) << 12) | vlanId & 0x0FFF;
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
