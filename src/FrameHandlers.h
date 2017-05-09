/********************************
 * KIV/PSI: VTP node            *
 * Semestral work               *
 *                              *
 * Author: Martin UBL           *
 *         A16N0026P            *
 *         ublm@students.zcu.cz *
 ********************************/

#ifndef FRAMEHANDLERS_H
#define FRAMEHANDLERS_H

#include <pcap.h>

#include "Singleton.h"
#include "VTPDefs.h"
#include "GUIDefs.h"

/*
 * Class maintaining all VTP frame-related stuff
 */
class FrameHandlerService : public CSingleton<FrameHandlerService>
{
    private:
        // current VLAN database revision
        uint32_t m_currentRevision;
        // last MD5 digest received
        uint8_t m_lastDigest[VTP_MD5_LENGTH];
        // last timestamp received
        uint8_t m_lastUpdateTimestamp[VTP_TIMESTAMP_LENGTH];
        // last updater identity
        uint32_t m_lastUpdaterIdentity;

    protected:
        // handles incoming summary advertisement
        bool HandleSummaryAdvertisement(VTPHeader* header, SummaryAdvertPacketBody* frame, size_t dataLen, FrameDumpContents* dump);
        // handles incoming subset advertisement
        bool HandleSubsetAdvertisement(VTPHeader* header, SubsetAdvertPacketBody* frame, size_t dataLen, FrameDumpContents* dump);
        // handles incoming advertisement request
        bool HandleAdvertisementRequest(VTPHeader* header, AdvertRequestPacketBody* frame, size_t dataLen, FrameDumpContents* dump);
        // handles incoming join message
        bool HandleJoin(VTPHeader* header, JoinPacketBody* frame, size_t dataLen, FrameDumpContents* dump);

        // fills VLAN info block to vector to be sent in subset advertisement frame
        void FillVLANInfoBlock(VLANRecord* vlan, std::vector<uint8_t> &target, FrameDumpContents* dump, size_t& sBase);

    public:
        FrameHandlerService();

        // handles incoming frame
        bool HandleIncoming(pcap_pkthdr *header, const uint8_t* data);

        // sends advertisement request
        void SendAdvertRequest(uint32_t startRevision = 0);
        // sends VLAN database (summary + subset advertisements)
        void SendVLANDatabase();
        // sends just summary advertisement
        void SendSummary(uint8_t followers = 0);
        // sends join message
        void SendJoin();
};

#define sFrameHandler FrameHandlerService::getInstance()

template<typename T> void ToNetworkEndianity(T* pkt);
template<typename T> void ToHostEndianity(T* pkt);

#endif
