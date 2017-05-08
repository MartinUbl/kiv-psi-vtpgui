#include "general.h"
#include "FrameHandlers.h"

#include "NetworkDefs.h"
#include "VTPDefs.h"
#include "VLANDatabase.h"
#include "Network.h"

#include "Globals.h"

#include <openssl/md5.h>
#include <sstream>
#include <iomanip>

template<> void ToNetworkEndianity<SummaryAdvertPacketBody>(SummaryAdvertPacketBody* pkt)
{
    pkt->revision = htonl(pkt->revision);
    pkt->updater_id = htonl(pkt->updater_id);
}

template<> void ToHostEndianity<SummaryAdvertPacketBody>(SummaryAdvertPacketBody* pkt)
{
    pkt->revision = ntohl(pkt->revision);
    pkt->updater_id = ntohl(pkt->updater_id);
}

template<> void ToNetworkEndianity<SubsetAdvertPacketBody>(SubsetAdvertPacketBody* pkt)
{
    pkt->revision = htonl(pkt->revision);
}

template<> void ToHostEndianity<SubsetAdvertPacketBody>(SubsetAdvertPacketBody* pkt)
{
    pkt->revision = ntohl(pkt->revision);
}

template<> void ToNetworkEndianity<AdvertRequestPacketBody>(AdvertRequestPacketBody* pkt)
{
    pkt->start_revision = htonl(pkt->start_revision);
}

template<> void ToHostEndianity<AdvertRequestPacketBody>(AdvertRequestPacketBody* pkt)
{
    pkt->start_revision = ntohl(pkt->start_revision);
}

template<> void ToNetworkEndianity<SubsetVLANInfoBody>(SubsetVLANInfoBody* pkt)
{
    pkt->isl_vlan_id = htons(pkt->isl_vlan_id);
    pkt->mtu_size = htons(pkt->mtu_size);
    pkt->index80210 = htonl(pkt->index80210);
}

template<> void ToHostEndianity<SubsetVLANInfoBody>(SubsetVLANInfoBody* pkt)
{
    pkt->isl_vlan_id = ntohs(pkt->isl_vlan_id);
    pkt->mtu_size = ntohs(pkt->mtu_size);
    pkt->index80210 = ntohl(pkt->index80210);
}

FrameHandlerService::FrameHandlerService()
{
    m_currentRevision = 0;
}

#define MYSTERY_DATA_LEN 8

int8_t vtp_generate_md5(char *secret, uint32_t updater, uint32_t revision, char *domain, uint8_t dom_len, uint8_t *vlans, uint16_t vlans_len, u_int8_t *md5, uint8_t version, char* timestamp)
{
    uint8_t* data, md5_secret[MD5_DIGEST_LENGTH];
    VTPHeader* vtp_hdr;
    SummaryAdvertPacketBody* vtp_summ;

    if ((data = (uint8_t*)calloc(1, (MD5_DIGEST_LENGTH + sizeof(VTPHeader) + sizeof(SummaryAdvertPacketBody) + MYSTERY_DATA_LEN + vlans_len + MD5_DIGEST_LENGTH))) == NULL)
        return -1;

    if (secret)
    {
        MD5_CTX c;

        MD5_Init(&c);
        MD5_Update(&c, secret, strlen(secret));
        MD5_Final(md5_secret, &c);

        memcpy(data, md5_secret, MD5_DIGEST_LENGTH);
    }

    vtp_hdr = (VTPHeader*)(data + MD5_DIGEST_LENGTH);
    vtp_summ = (SummaryAdvertPacketBody*)(data + MD5_DIGEST_LENGTH + sizeof(VTPHeader));

    vtp_hdr->version = version;
    vtp_hdr->code = VTP_MSG_SUMMARY_ADVERT;
    vtp_hdr->domain_len = dom_len;
    memcpy(vtp_hdr->domain_name, domain, dom_len);

    vtp_summ->updater_id = htonl(updater);
    vtp_summ->revision = htonl(revision);
    memcpy(vtp_summ->update_timestamp, timestamp, VTP_TIMESTAMP_LEN);

    const uint8_t mysteryData[MYSTERY_DATA_LEN] = { 0, 0, 0, 1, 6, 1, 0, 2 };
    //const uint8_t mysteryData[MYSTERY_DATA_LEN] = { 1, 1, 0, 2, 0 };
    memcpy((void *)(data + MD5_DIGEST_LENGTH + sizeof(VTPHeader) + sizeof(SummaryAdvertPacketBody)), mysteryData, MYSTERY_DATA_LEN);

    if (vlans_len)
        memcpy((void *)(data + MD5_DIGEST_LENGTH + sizeof(VTPHeader) + sizeof(SummaryAdvertPacketBody) + MYSTERY_DATA_LEN), vlans, vlans_len);

    if (secret)
        memcpy((void *)(data + MD5_DIGEST_LENGTH + sizeof(VTPHeader) + sizeof(SummaryAdvertPacketBody) + MYSTERY_DATA_LEN + vlans_len), md5_secret, MD5_DIGEST_LENGTH);

    MD5_CTX ctx;

    MD5_Init(&ctx);
    MD5_Update(&ctx, data, (MD5_DIGEST_LENGTH + sizeof(VTPHeader) + sizeof(SummaryAdvertPacketBody) + MYSTERY_DATA_LEN + vlans_len + MD5_DIGEST_LENGTH));
    MD5_Final(md5, &ctx);

    free(data);

    return 0;
}

#define ADD_DUMP_ENTRY(dump,k,v,s,len) { dump->tableRows.push_back(std::make_pair(k, v)); dump->dumpStartEnds.push_back(std::make_pair(s, s+len)); s += len; }

bool FrameHandlerService::HandleSummaryAdvertisement(VTPHeader* header, SummaryAdvertPacketBody* frame, size_t dataLen, FrameDumpContents* dump)
{
    ToHostEndianity(frame);

    size_t sBase = 0;

    ADD_DUMP_ENTRY(dump, "VTP version", std::to_string(header->version), sBase, 1);
    ADD_DUMP_ENTRY(dump, "VTP code", std::to_string(header->code), sBase, 1);
    ADD_DUMP_ENTRY(dump, "Followers", std::to_string(header->followers), sBase, 1);
    ADD_DUMP_ENTRY(dump, "Domain length", std::to_string(header->domain_len), sBase, 1);
    ADD_DUMP_ENTRY(dump, "Domain name", std::string((const char*)header->domain_name, MAX_VTP_DOMAIN_LENGTH), sBase, MAX_VTP_DOMAIN_LENGTH);

    ADD_DUMP_ENTRY(dump, "Revision", std::to_string(frame->revision), sBase, 4);

    char updater[32];
    inet_ntop(AF_INET, &frame->updater_id, updater, 32);

    ADD_DUMP_ENTRY(dump, "Updater ID", updater, sBase, 4);
    ADD_DUMP_ENTRY(dump, "Timestamp", std::string((const char*)frame->update_timestamp, VTP_TIMESTAMP_LENGTH), sBase, VTP_TIMESTAMP_LENGTH);

    std::ostringstream ss;

    size_t pp;
    for (pp = 0; pp < VTP_MD5_LENGTH - 1; pp++)
        ss << std::setfill('0') << std::setw(2) << std::hex << std::uppercase << (int)frame->md5_digest[pp] << " ";
    ss << std::setfill('0') << std::setw(2) << std::hex << std::uppercase << (int)frame->md5_digest[pp];

    ADD_DUMP_ENTRY(dump, "MD5 Digest", ss.str().c_str(), sBase, VTP_MD5_LENGTH);

    if (dataLen + 4 > sizeof(SummaryAdvertPacketBody))
    {
        uint8_t* addData = (uint8_t*)(frame + 1);

        if (header->version == 1)
        {
            ADD_DUMP_ENTRY(dump, "Unknown", std::to_string(addData[0]), sBase, 1);
            ADD_DUMP_ENTRY(dump, "Unknown", std::to_string(addData[1]), sBase, 1);
            ADD_DUMP_ENTRY(dump, "VTP Pruning", std::to_string(addData[2]), sBase, 1);
            ADD_DUMP_ENTRY(dump, "Unknown", std::to_string(addData[3]), sBase, 1);
            ADD_DUMP_ENTRY(dump, "Unknown", std::to_string(addData[4]), sBase, 1);
        }
        else if (header->version == 2)
        {
            ADD_DUMP_ENTRY(dump, "Unknown", std::to_string(addData[0]), sBase, 1);
            ADD_DUMP_ENTRY(dump, "Unknown", std::to_string(addData[1]), sBase, 1);
            ADD_DUMP_ENTRY(dump, "Unknown", std::to_string(addData[2]), sBase, 1);
            ADD_DUMP_ENTRY(dump, "Unknown", std::to_string(addData[3]), sBase, 1);

            size_t pos = 4;
            while (pos + 1 < dataLen - sizeof(SummaryAdvertPacketBody) + 4)
            {
                uint8_t type = addData[pos];
                uint8_t len = addData[pos + 1];

                uint16_t val = ntohs(*((uint16_t*)(&addData[pos + 2])));

                if (type == 6)
                    ADD_DUMP_ENTRY(dump, "Feature", "VTP Pruning (6)", sBase, 1)
                else
                    ADD_DUMP_ENTRY(dump, "Feature", std::to_string(type), sBase, 1)

                    ADD_DUMP_ENTRY(dump, "Length", std::to_string(len), sBase, 1);
                ADD_DUMP_ENTRY(dump, "Value", std::to_string(val), sBase, 2);

                pos += 2 + len * 2;
            }
        }
    }

    if (frame->revision > m_currentRevision)
    {
        std::cout << "VTP: received newer revision summary rev: " << frame->revision << " (local: " << m_currentRevision << ")" << std::endl;

        if (header->followers == 0)
        {
            // TODO: signal GUI about newer revision
        }

        memcpy(m_lastDigest, frame->md5_digest, MD5_DIGEST_LEN);
        memcpy(m_lastUpdateTimestamp, frame->update_timestamp, VTP_TIMESTAMP_LEN);
        m_lastUpdaterIdentity = frame->updater_id;
    }
    else if (frame->revision < m_currentRevision)
        std::cout << "VTP: received older revision summary rev: " << frame->revision << " (local: " << m_currentRevision << "); the network is in inconsistent state" << std::endl;
    else
    {
        std::cout << "VTP: received current revision summary rev: " << frame->revision << std::endl;

        memcpy(m_lastDigest, frame->md5_digest, MD5_DIGEST_LEN);
        memcpy(m_lastUpdateTimestamp, frame->update_timestamp, VTP_TIMESTAMP_LEN);
        m_lastUpdaterIdentity = frame->updater_id;
    }

    return true;
}

bool FrameHandlerService::HandleSubsetAdvertisement(VTPHeader* header, SubsetAdvertPacketBody* frame, size_t dataLen, FrameDumpContents* dump)
{
    ToHostEndianity(frame);

    size_t sBase = 0;

    ADD_DUMP_ENTRY(dump, "VTP version", std::to_string(header->version), sBase, 1);
    ADD_DUMP_ENTRY(dump, "VTP code", std::to_string(header->code), sBase, 1);
    ADD_DUMP_ENTRY(dump, "Sequence number", std::to_string(header->sequence_nr), sBase, 1);
    ADD_DUMP_ENTRY(dump, "Domain length", std::to_string(header->domain_len), sBase, 1);
    ADD_DUMP_ENTRY(dump, "Domain name", std::string((const char*)header->domain_name, MAX_VTP_DOMAIN_LENGTH), sBase, MAX_VTP_DOMAIN_LENGTH);

    ADD_DUMP_ENTRY(dump, "Revision", std::to_string(frame->revision), sBase, 4);

    // if received revision is higher or equal than our revision, read contents and update our VLAN database
    if (frame->revision >= m_currentRevision)
    {
        SubsetVLANInfoBody* cur;
        VLANRecord* vlan;
        std::string name;
        std::vector<SubsetVLANInfoBody*> readVlans;
        std::set<uint16_t> vlanIds;

        // iterate while we are still within limits
        size_t offset = 0;
        while (offset < dataLen)
        {
            // "cut" next chunk and convert to host endianity
            cur = (SubsetVLANInfoBody*)((&frame->data) + offset);
            ToHostEndianity(cur);

            // extract VLAN name
            name = std::string((const char*)&cur->data, (size_t)cur->name_length);

            int name_padded = ((int)(cur->name_length / 4) + (cur->name_length % 4 ? 1 : 0)) * 4;

            vlanIds.insert(cur->isl_vlan_id);

            // add or update VLAN
            vlan = sVLANDatabase->GetVLANById(cur->isl_vlan_id);
            if (!vlan)
            {
                sVLANDatabase->AddVLAN(cur->isl_vlan_id, cur->type, cur->status, cur->mtu_size, cur->index80210, name.c_str());
                vlan = sVLANDatabase->GetVLANById(cur->isl_vlan_id);
            }
            else
                sVLANDatabase->UpdateVLAN(cur->isl_vlan_id, cur->type, cur->status, cur->mtu_size, cur->index80210, name.c_str());

            ADD_DUMP_ENTRY(dump, "VLAN " + std::to_string(cur->isl_vlan_id) + " - block length", std::to_string(cur->length), sBase, 1);
            ADD_DUMP_ENTRY(dump, "VLAN " + std::to_string(cur->isl_vlan_id) + " - status", std::to_string(cur->status), sBase, 1);
            ADD_DUMP_ENTRY(dump, "VLAN " + std::to_string(cur->isl_vlan_id) + " - type", std::to_string(cur->type), sBase, 1);
            ADD_DUMP_ENTRY(dump, "VLAN " + std::to_string(cur->isl_vlan_id) + " - name length", std::to_string(cur->name_length), sBase, 1);
            ADD_DUMP_ENTRY(dump, "VLAN " + std::to_string(cur->isl_vlan_id) + " - ISL VLAN ID", std::to_string(cur->isl_vlan_id), sBase, 2);
            ADD_DUMP_ENTRY(dump, "VLAN " + std::to_string(cur->isl_vlan_id) + " - MTU size", std::to_string(cur->mtu_size), sBase, 2);
            ADD_DUMP_ENTRY(dump, "VLAN " + std::to_string(cur->isl_vlan_id) + " - 802.10 index", std::to_string(cur->index80210), sBase, 4);
            ADD_DUMP_ENTRY(dump, "VLAN " + std::to_string(cur->isl_vlan_id) + " - name", name.c_str(), sBase, name_padded);

            vlan->features.clear();
            for (size_t g = offset + sizeof(SubsetVLANInfoBody) - 1 + name_padded; g < offset + cur->length; g += 4)
            {
                uint8_t col_id = (*(&frame->data + g));
                uint8_t par = (*(&frame->data + g + 1));
                uint16_t val = ntohs((*(uint16_t*)(&frame->data + g + 2)));

                if (col_id == VLAN_FEAT_NONE || col_id >= MAX_VLAN_FEATURE)
                    ADD_DUMP_ENTRY(dump, "VLAN " + std::to_string(cur->isl_vlan_id) + " - feature", std::to_string(col_id), sBase, 1)
                else
                    ADD_DUMP_ENTRY(dump, "VLAN " + std::to_string(cur->isl_vlan_id) + " - feature", VLANFeatureNames[col_id] + (" (" + std::to_string(col_id)) + ")", sBase, 1)
                ADD_DUMP_ENTRY(dump, "VLAN " + std::to_string(cur->isl_vlan_id) + " - feature len", std::to_string(par), sBase, 1);
                ADD_DUMP_ENTRY(dump, "VLAN " + std::to_string(cur->isl_vlan_id) + " - feature value", std::to_string(val), sBase, 2);

                // TODO: elaborate with "par" value - 1 probably means single value, 2 probably means two values padded to next field
                if (col_id == VLAN_FEAT_TRANS1 && par == 2)
                {
                    g += 4;
                    uint16_t trans1 = ntohs((*(uint16_t*)(&frame->data + g + 0)));
                    uint16_t trans2 = ntohs((*(uint16_t*)(&frame->data + g + 2)));

                    ADD_DUMP_ENTRY(dump, "VLAN " + std::to_string(cur->isl_vlan_id) + " - feature value", std::to_string(trans1), sBase, 2);
                    ADD_DUMP_ENTRY(dump, "VLAN " + std::to_string(cur->isl_vlan_id) + " - feature value", std::to_string(trans2), sBase, 2);

                    vlan->features[VLAN_FEAT_TRANS1] = trans1;
                    vlan->features[VLAN_FEAT_TRANS2] = trans2;
                    continue;
                }

                if (col_id == VLAN_FEAT_NONE || col_id >= MAX_VLAN_FEATURE)
                    std::cerr << "Unknown VLAN feature " << (int)col_id << " (par: " << (int)par << ") with value " << (int)val << " received" << std::endl;
                else
                    vlan->features[col_id] = val;
            }

            offset += cur->length;
        }

        // remove VLANs not present in subset advert packet
        sVLANDatabase->Flush(vlanIds);

        // update local revision number
        m_currentRevision = frame->revision;


        MainWindow* mw = sAppGlobals->g_MainWindow;

        mw->ClearVLANView();
        VLANMap const& vlanmap = sVLANDatabase->GetVLANMap();

        for (auto pr : vlanmap)
            mw->AddVLAN(pr.second->id, pr.second->name, pr.second);

        // TODO: fix MD5 input, otherwise the MD5 generated is different from MD5 received!
        uint8_t md5target[MD5_DIGEST_LENGTH];
        vtp_generate_md5(sAppGlobals->g_VTPPassword.length() == 0 ? nullptr : sAppGlobals->g_VTPPassword.c_str(), m_lastUpdaterIdentity, frame->revision, (char*)header->domain_name, header->domain_len, &frame->data, (uint16_t)(dataLen - 4), md5target, header->version, (char*)m_lastUpdateTimestamp);

        for (size_t j = 0; j < MD5_DIGEST_LENGTH; j++)
            printf("%.2X : %.2X\n", m_lastDigest[j], md5target[j]);
        printf("\n");
    }

    return true;
}

bool FrameHandlerService::HandleAdvertisementRequest(VTPHeader* header, AdvertRequestPacketBody* frame, size_t dataLen, FrameDumpContents* dump)
{
    ToHostEndianity(frame);

    return true;
}

void FrameHandlerService::SendAdvertRequest(uint32_t startRevision)
{
    uint16_t dataSize = sizeof(VTPHeader) + sizeof(AdvertRequestPacketBody);

    uint8_t *data = new uint8_t[dataSize];
    VTPHeader* header = (VTPHeader*)data;
    AdvertRequestPacketBody* frame = (AdvertRequestPacketBody*)(data + sizeof(VTPHeader));

    header->version = 2;
    header->code = VTP_MSG_ADVERT_REQUEST;
    header->reserved = 0;
    header->domain_len = (uint8_t)sAppGlobals->g_VTPDomain.length();
    memset(header->domain_name, 0, MAX_VTP_DOMAIN_LENGTH);
    strncpy((char*)header->domain_name, sAppGlobals->g_VTPDomain.c_str(), MAX_VTP_DOMAIN_LENGTH);

    frame->start_revision = startRevision;


    std::ostringstream ss;

    FrameDumpContents* dump = new FrameDumpContents;
    for (size_t pp = 0; pp < dataSize; pp++)
        ss << std::setfill('0') << std::setw(2) << std::hex << std::uppercase << (int)data[pp] << " ";

    dump->contents = ss.str().c_str();
    size_t sBase = 0;

    ADD_DUMP_ENTRY(dump, "VTP version", std::to_string(header->version), sBase, 1);
    ADD_DUMP_ENTRY(dump, "VTP code", std::to_string(header->code), sBase, 1);
    ADD_DUMP_ENTRY(dump, "(Reserved)", std::to_string(header->reserved), sBase, 1);
    ADD_DUMP_ENTRY(dump, "Domain length", std::to_string(header->domain_len), sBase, 1);
    ADD_DUMP_ENTRY(dump, "Domain name", std::string((const char*)header->domain_name, MAX_VTP_DOMAIN_LENGTH), sBase, MAX_VTP_DOMAIN_LENGTH);

    ADD_DUMP_ENTRY(dump, "Start revision", std::to_string(frame->start_revision), sBase, 4);

    sAppGlobals->g_MainWindow->AddTrafficEntry((uint64_t)time(nullptr), true, header->code, "3 - Advertisement Request", dump);

    ToNetworkEndianity(frame);

    sNetwork->SendUsingTemplate(data, dataSize);

    delete[] data;
}

#define VEC_ADD_1B(x) { target.push_back(x); }
#define VEC_ADD_2B(x) { target.push_back((x >> 8) & 0xFF); target.push_back(x & 0xFF); }
#define VEC_ADD_4B(x) { target.push_back((x >> 24) & 0xFF); target.push_back((x >> 16) & 0xFF); target.push_back((x >> 8) & 0xFF); target.push_back(x & 0xFF); }

void FrameHandlerService::FillVLANInfoBlock(VLANRecord* vlan, std::vector<uint8_t> &target, FrameDumpContents* dump, size_t& sBase)
{
    size_t i;

    size_t sizePos = target.size();

    // size placeholder
    VEC_ADD_1B(0);

    VEC_ADD_1B(vlan->status);
    VEC_ADD_1B(vlan->type);
    VEC_ADD_1B(((uint8_t)vlan->name.size()));

    VEC_ADD_2B(vlan->id);
    VEC_ADD_2B(vlan->mtu);

    VEC_ADD_4B(vlan->index80210);

    // insert name
    for (i = 0; i < vlan->name.size(); i++)
        VEC_ADD_1B(vlan->name[i]);

    size_t rem = 0;
    // pad to multiple of 4
    if ((vlan->name.size() % 4) != 0)
    {
        rem = 3 - (vlan->name.size() % 4);
        for (i = 0; i <= rem; i++)
            VEC_ADD_1B(0);
    }

    // append all features
    for (std::pair<uint16_t, uint16_t> ftr : vlan->features)
    {
        // exception for TRANS1 and TRANS2 presence - newer Cisco switches sends this information in one feature field of length 2
        if (ftr.first == VLAN_FEAT_TRANS1 && vlan->features.find(VLAN_FEAT_TRANS2) != vlan->features.end())
        {
            VEC_ADD_1B(ftr.first);
            VEC_ADD_1B(0x02);
            VEC_ADD_2B(0x00);
            VEC_ADD_2B(ftr.second);
            VEC_ADD_2B(vlan->features[VLAN_FEAT_TRANS2]);
            continue;
        }

        if (ftr.first == VLAN_FEAT_TRANS2 && vlan->features.find(VLAN_FEAT_TRANS1) != vlan->features.end())
            continue;

        VEC_ADD_1B(ftr.first);
        VEC_ADD_1B(0x01);
        VEC_ADD_2B(ftr.second);
    }

    target[sizePos] = (uint8_t)(target.size() - sizePos);

    ADD_DUMP_ENTRY(dump, "VLAN " + std::to_string(vlan->id) + " - block length", std::to_string(target.size() - sizePos), sBase, 1);
    ADD_DUMP_ENTRY(dump, "VLAN " + std::to_string(vlan->id) + " - status", std::to_string(vlan->status), sBase, 1);
    ADD_DUMP_ENTRY(dump, "VLAN " + std::to_string(vlan->id) + " - type", std::to_string(vlan->type), sBase, 1);
    ADD_DUMP_ENTRY(dump, "VLAN " + std::to_string(vlan->id) + " - name length", std::to_string(vlan->name.size()), sBase, 1);
    ADD_DUMP_ENTRY(dump, "VLAN " + std::to_string(vlan->id) + " - ISL VLAN ID", std::to_string(vlan->id), sBase, 2);
    ADD_DUMP_ENTRY(dump, "VLAN " + std::to_string(vlan->id) + " - MTU size", std::to_string(vlan->mtu), sBase, 2);
    ADD_DUMP_ENTRY(dump, "VLAN " + std::to_string(vlan->id) + " - 802.10 index", std::to_string(vlan->index80210), sBase, 4);
    ADD_DUMP_ENTRY(dump, "VLAN " + std::to_string(vlan->id) + " - name", vlan->name.c_str(), sBase, (vlan->name.size() + rem));

    for (std::pair<uint16_t, uint16_t> ftr : vlan->features)
    {
        if (ftr.first == VLAN_FEAT_TRANS1 && vlan->features.find(VLAN_FEAT_TRANS2) != vlan->features.end())
        {
            ADD_DUMP_ENTRY(dump, "VLAN " + std::to_string(vlan->id) + " - feature", VLANFeatureNames[ftr.first] + (" (" + std::to_string(ftr.first)) + ")", sBase, 1);
            ADD_DUMP_ENTRY(dump, "VLAN " + std::to_string(vlan->id) + " - feature len", std::to_string(0x02), sBase, 1);
            ADD_DUMP_ENTRY(dump, "VLAN " + std::to_string(vlan->id) + " - feature value", std::to_string(0x00), sBase, 2);

            ADD_DUMP_ENTRY(dump, "VLAN " + std::to_string(vlan->id) + " - feature value", std::to_string(ftr.second), sBase, 2);
            ADD_DUMP_ENTRY(dump, "VLAN " + std::to_string(vlan->id) + " - feature value", std::to_string(vlan->features[VLAN_FEAT_TRANS2]), sBase, 2);
            continue;
        }

        if (ftr.first == VLAN_FEAT_TRANS2 && vlan->features.find(VLAN_FEAT_TRANS1) != vlan->features.end())
            continue;

        if (ftr.first == VLAN_FEAT_NONE || ftr.first >= MAX_VLAN_FEATURE)
            ADD_DUMP_ENTRY(dump, "VLAN " + std::to_string(vlan->id) + " - feature", std::to_string(ftr.first), sBase, 1)
        else
            ADD_DUMP_ENTRY(dump, "VLAN " + std::to_string(vlan->id) + " - feature", VLANFeatureNames[ftr.first] + (" (" + std::to_string(ftr.first)) + ")", sBase, 1)
        ADD_DUMP_ENTRY(dump, "VLAN " + std::to_string(vlan->id) + " - feature len", std::to_string(0x01), sBase, 1);
        ADD_DUMP_ENTRY(dump, "VLAN " + std::to_string(vlan->id) + " - feature value", std::to_string(ftr.second), sBase, 2);
    }
}

void FrameHandlerService::SendVLANDatabase()
{
    // contains probably more settings, for now, we know about pruning setting (06 01 00 02 = pruning off)
    const uint8_t mysteryData[MYSTERY_DATA_LEN] = { 0, 0, 0, 1, 6, 1, 0, 2 };

    uint16_t dataSize = sizeof(VTPHeader) + sizeof(SummaryAdvertPacketBody) + MYSTERY_DATA_LEN;

    uint8_t *data = new uint8_t[dataSize];
    VTPHeader* header = (VTPHeader*)data;
    SummaryAdvertPacketBody* frame = (SummaryAdvertPacketBody*)(data + sizeof(VTPHeader));
    memcpy(data + sizeof(VTPHeader) + sizeof(SummaryAdvertPacketBody), mysteryData, MYSTERY_DATA_LEN);

    header->version = 2;
    header->code = VTP_MSG_SUMMARY_ADVERT;
    header->followers = 1; // TODO: count VLANs and split to more messages if needed
    header->domain_len = (uint8_t)sAppGlobals->g_VTPDomain.length();
    memset(header->domain_name, 0, MAX_VTP_DOMAIN_LENGTH);
    strncpy((char*)header->domain_name, sAppGlobals->g_VTPDomain.c_str(), MAX_VTP_DOMAIN_LENGTH);

    frame->revision = m_currentRevision;
    frame->updater_id = m_lastUpdaterIdentity;
    memcpy(frame->update_timestamp, m_lastUpdateTimestamp, VTP_TIMESTAMP_LENGTH);
    memcpy(frame->md5_digest, m_lastDigest, VTP_MD5_LENGTH);

    std::ostringstream ss;

    FrameDumpContents* dump = new FrameDumpContents;
    for (size_t pp = 0; pp < dataSize; pp++)
        ss << std::setfill('0') << std::setw(2) << std::hex << std::uppercase << (int)data[pp] << " ";

    dump->contents = ss.str().c_str();
    size_t sBase = 0;

    ADD_DUMP_ENTRY(dump, "VTP version", std::to_string(header->version), sBase, 1);
    ADD_DUMP_ENTRY(dump, "VTP code", std::to_string(header->code), sBase, 1);
    ADD_DUMP_ENTRY(dump, "Followers", std::to_string(header->followers), sBase, 1);
    ADD_DUMP_ENTRY(dump, "Domain length", std::to_string(header->domain_len), sBase, 1);
    ADD_DUMP_ENTRY(dump, "Domain name", std::string((const char*)header->domain_name, MAX_VTP_DOMAIN_LENGTH), sBase, MAX_VTP_DOMAIN_LENGTH);

    ADD_DUMP_ENTRY(dump, "Revision", std::to_string(frame->revision), sBase, 4);

    char updater[32];
    inet_ntop(AF_INET, &frame->updater_id, updater, 32);

    ADD_DUMP_ENTRY(dump, "Updater ID", updater, sBase, 4);
    ADD_DUMP_ENTRY(dump, "Timestamp", std::string((const char*)frame->update_timestamp, VTP_TIMESTAMP_LENGTH), sBase, VTP_TIMESTAMP_LENGTH);

    ss.str(std::string());

    size_t pp;
    for (pp = 0; pp < VTP_MD5_LENGTH - 1; pp++)
        ss << std::setfill('0') << std::setw(2) << std::hex << std::uppercase << (int)frame->md5_digest[pp] << " ";
    ss << std::setfill('0') << std::setw(2) << std::hex << std::uppercase << (int)frame->md5_digest[pp];

    ADD_DUMP_ENTRY(dump, "MD5 Digest", ss.str().c_str(), sBase, VTP_MD5_LENGTH);

    sAppGlobals->g_MainWindow->AddTrafficEntry((uint64_t)time(nullptr), true, header->code, "1 - Summary Advertisement", dump);

    ToNetworkEndianity(frame);

    sNetwork->SendUsingTemplate(data, dataSize);

    delete[] data;

    ///////// Subset Advert

    std::vector<uint8_t> vlanInfo;
    size_t fixedSize = sizeof(VTPHeader) + sizeof(SubsetAdvertPacketBody) - 1; /* subtract dummy placeholder */
    data = new uint8_t[fixedSize];

    header = (VTPHeader*)data;
    SubsetAdvertPacketBody* frame2 = (SubsetAdvertPacketBody*)(data + sizeof(VTPHeader));

    header->version = 2;
    header->code = VTP_MSG_SUBSET_ADVERT;
    header->sequence_nr = 1; // TODO: count VLANs and split to more messages if needed
    header->domain_len = (uint8_t)sAppGlobals->g_VTPDomain.length();
    memset(header->domain_name, 0, MAX_VTP_DOMAIN_LENGTH);
    strncpy((char*)header->domain_name, sAppGlobals->g_VTPDomain.c_str(), MAX_VTP_DOMAIN_LENGTH);

    frame2->revision = m_currentRevision;

    ss.str(std::string());

    dump = new FrameDumpContents;
    sBase = 0;

    ADD_DUMP_ENTRY(dump, "VTP version", std::to_string(header->version), sBase, 1);
    ADD_DUMP_ENTRY(dump, "VTP code", std::to_string(header->code), sBase, 1);
    ADD_DUMP_ENTRY(dump, "Sequence number", std::to_string(header->followers), sBase, 1);
    ADD_DUMP_ENTRY(dump, "Domain length", std::to_string(header->domain_len), sBase, 1);
    ADD_DUMP_ENTRY(dump, "Domain name", std::string((const char*)header->domain_name, MAX_VTP_DOMAIN_LENGTH), sBase, MAX_VTP_DOMAIN_LENGTH);

    ADD_DUMP_ENTRY(dump, "Revision", std::to_string(frame2->revision), sBase, 4);

    VLANMap const& vlans = sVLANDatabase->GetVLANMap();

    for (auto vpair : vlans)
        FillVLANInfoBlock(vpair.second, vlanInfo, dump, sBase);

    dataSize = (uint16_t)(fixedSize + vlanInfo.size());
    uint8_t *finalData = new uint8_t[dataSize];
    memcpy(finalData, data, fixedSize);
    memcpy(finalData + fixedSize, &vlanInfo[0], vlanInfo.size());

    for (uint16_t pp = 0; pp < dataSize; pp++)
        ss << std::setfill('0') << std::setw(2) << std::hex << std::uppercase << (int)finalData[pp] << " ";

    dump->contents = ss.str().c_str();

    sAppGlobals->g_MainWindow->AddTrafficEntry((uint64_t)time(nullptr), true, header->code, "2 - Subset Advertisement", dump);

    ToNetworkEndianity(frame2);

    sNetwork->SendUsingTemplate(finalData, dataSize);

    delete[] data;
    delete[] finalData;
}

bool FrameHandlerService::HandleIncoming(pcap_pkthdr *header, const uint8_t* data)
{
    uint16_t vlanId = 0;
    size_t i;

    EthernetHeader* hdr = (EthernetHeader*)data;
    LLCHeader* llchdr;

    // 802.1Q tagged frame (port is somehow marked as access port)
    if (ntohs(hdr->ether_type) == DOT1Q_ETHERTYPE)
    {
        Dot1QHeader* dhdr = (Dot1QHeader*)hdr;
        vlanId = ntohs(dhdr->tci) & 0xFFF; // 12 last bits are vlanId

        llchdr = (LLCHeader*)(data + sizeof(Dot1QHeader));

        if (!sNetwork->HasSendingTemplate())
            sNetwork->SetSendingTemplate(SendingTemplate::NET_SEND_TEMPLATE_DOT1Q, vlanId);
    }
    else // untagged frame (port is marked as trunk port)
    {
        llchdr = (LLCHeader*)(data + sizeof(EthernetHeader));

        if (!sNetwork->HasSendingTemplate())
            sNetwork->SetSendingTemplate(SendingTemplate::NET_SEND_TEMPLATE_8023, 0);
    }

    // VTP frames always have LLC header with SNAP extension

    // parse LLC header

    if (llchdr->dsap != SNAP_DSAP_IDENTIFIER || llchdr->ssap != SNAP_SSAP_IDENTIFIER || llchdr->control != SNAP_CONTROL_VALUE)
        return false;

    SNAPHeader* snaphdr = (SNAPHeader*)(((uint8_t*)llchdr) + sizeof(LLCHeader));

    for (i = 0; i < SNAP_OUI_LEN; i++)
    {
        if (snaphdr->oui[i] != OUI_Cisco[i])
            return false;
    }

    if (ntohs(snaphdr->protocol_id) != VTP_IDENTIFIER)
        return false;

    // we are the senders of this frame
    if (memcmp(hdr->src_mac, sAppGlobals->g_MACAddress, MAC_ADDR_LENGTH) == 0)
        return false;

    // is VTP

    VTPHeader* vtphdr = (VTPHeader*)(((uint8_t*)snaphdr) + sizeof(SNAPHeader));
    uint8_t* vtpdata = (uint8_t*)(((uint8_t*)vtphdr) + sizeof(VTPHeader));

    // TODO: configurable domain
    if (strncmp((const char*)vtphdr->domain_name, sAppGlobals->g_VTPDomain.c_str(), vtphdr->domain_len) != 0)
    {
        // VTP format is probably valid, but the message is not for us (our domain); ignore
        return true;
    }

    // subtract overhead length from packet length to obtain raw data length
    size_t dataLen = header->caplen - (vtpdata - data) - 4;

    /*
    for (size_t pp = 0; pp < dataLen; pp++)
    {
        if (vtpdata[pp] >= 'a' && vtpdata[pp] <= 'z')
            printf(" %c ", vtpdata[pp]);
        else
            printf("%.2X ", vtpdata[pp]);

        if (pp != 0 && pp % 16 == 0)
            printf("\n");
    }

    printf("\n");
    printf("\n");
    */

    std::ostringstream ss;

    FrameDumpContents* dump = new FrameDumpContents;
    uint8_t* rawData = (uint8_t*)vtphdr;
    size_t rawLength = header->caplen - ((uint8_t*)vtphdr - data);
    for (size_t pp = 0; pp < rawLength; pp++)
        ss << std::setfill('0') << std::setw(2) << std::hex << std::uppercase << (int)rawData[pp] << " ";

    dump->contents = ss.str().c_str();

    switch (vtphdr->code)
    {
        case VTP_MSG_SUMMARY_ADVERT:
            HandleSummaryAdvertisement(vtphdr, reinterpret_cast<SummaryAdvertPacketBody*>(vtpdata), dataLen, dump);
            sAppGlobals->g_MainWindow->AddTrafficEntry((uint64_t)time(nullptr), false, vtphdr->code, "1 - Summary Advertisement", dump);
            break;
        case VTP_MSG_SUBSET_ADVERT:
            HandleSubsetAdvertisement(vtphdr, reinterpret_cast<SubsetAdvertPacketBody*>(vtpdata), dataLen, dump);
            sAppGlobals->g_MainWindow->AddTrafficEntry((uint64_t)time(nullptr), false, vtphdr->code, "2 - Subset Advertisement", dump);
            break;
        case VTP_MSG_ADVERT_REQUEST:
            HandleAdvertisementRequest(vtphdr, reinterpret_cast<AdvertRequestPacketBody*>(vtpdata), dataLen, dump);
            sAppGlobals->g_MainWindow->AddTrafficEntry((uint64_t)time(nullptr), false, vtphdr->code, "3 - Advertisement Request", dump);
            break;
        case VTP_MSG_JOIN:
        case VTP_MSG_TAKEOVER_REQUEST:
        case VTP_MSG_TAKEOVER_RESPONSE:
            break;
        default:
            std::cerr << "VTP: received unknown message with code " << (int)vtphdr->code << std::endl;
            break;
    }

    return true;
}
