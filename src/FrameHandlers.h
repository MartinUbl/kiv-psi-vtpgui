#ifndef FRAMEHANDLERS_H
#define FRAMEHANDLERS_H

#include <pcap.h>

#include "Singleton.h"
#include "VTPDefs.h"
#include "GUIDefs.h"

#define MD5_DIGEST_LEN 16
#define VTP_TIMESTAMP_LEN 12

class FrameHandlerService : public CSingleton<FrameHandlerService>
{
    private:
        uint32_t m_currentRevision;
        uint8_t m_lastDigest[MD5_DIGEST_LEN];
        uint8_t m_lastUpdateTimestamp[VTP_TIMESTAMP_LEN];
        uint32_t m_lastUpdaterIdentity;

    protected:
        bool HandleSummaryAdvertisement(VTPHeader* header, SummaryAdvertPacketBody* frame, size_t dataLen, FrameDumpContents* dump);
        bool HandleSubsetAdvertisement(VTPHeader* header, SubsetAdvertPacketBody* frame, size_t dataLen, FrameDumpContents* dump);
        bool HandleAdvertisementRequest(VTPHeader* header, AdvertRequestPacketBody* frame, size_t dataLen, FrameDumpContents* dump);

        void FillVLANInfoBlock(VLANRecord* vlan, std::vector<uint8_t> &target, FrameDumpContents* dump, size_t& sBase);

    public:
        FrameHandlerService();

        bool HandleIncoming(pcap_pkthdr *header, const uint8_t* data);

        void SendAdvertRequest(uint32_t startRevision = 0);
        void SendVLANDatabase();
        void SendSummary(uint8_t followers = 0);
};

#define sFrameHandler FrameHandlerService::getInstance()

template<typename T> void ToNetworkEndianity(T* pkt);
template<typename T> void ToHostEndianity(T* pkt);

#endif
