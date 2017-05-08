#ifndef VTP_DEFS_H
#define VTP_DEFS_H

// DSAP value used for VTP purposes (802.2 unnumbered PDU)
#define SNAP_DSAP_IDENTIFIER 0xAA
// SSAP value used for VTP purposes (802.2 unnumbered PDU)
#define SNAP_SSAP_IDENTIFIER 0xAA
// control field value we use (802.2 unnumbered PDU)
#define SNAP_CONTROL_VALUE 0x03

// put DSAP and SSAP those values together for easier comparation
#define SNAP_DSAP_SSAP_IDENTIFIER (SNAP_DSAP_IDENTIFIER << 8 | SNAP_SSAP_IDENTIFIER)

// 802.1Q EtherType (tagged communication)
#define DOT1Q_ETHERTYPE 0x8100
// VTP protocol identifier (SNAP field)
#define VTP_IDENTIFIER 0x2003

// OUI for Cisco used in SNAP extension field
const uint8_t OUI_Cisco[3] = { 0x00, 0x00, 0x0C };

#define SNAP_NOT_TAGGED_OFFSET 14
#define SNAP_TAGGED_OFFSET 18

#define VTP_ID_OFFSET 6
#define VTP_VERSION_OFFSET 8
#define VTP_MSGTYPE_OFFSET 9

// TPID value used in tagged 802.1Q traffic
#define DOT1Q_VTP_TPID 0x8100
// PCP (Priority Code Point) used in tagged 802.1Q traffic
#define DOT1Q_VTP_PCP 0
// DEI (Drop Eligible Indicator) used in tagged 802.1Q traffic
#define DOT1Q_VTP_DEI 0

const uint8_t VTP_Dest_MAC[6] = { 0x01, 0x00, 0x0c, 0xcc, 0xcc, 0xcc };

enum VTPMsgCode
{
    VTP_MSG_SUMMARY_ADVERT = 1,
    VTP_MSG_SUBSET_ADVERT = 2,
    VTP_MSG_ADVERT_REQUEST = 3,
    VTP_MSG_JOIN = 4,    // TODO: verify
    VTP_MSG_TAKEOVER_REQUEST = 5,
    VTP_MSG_TAKEOVER_RESPONSE = 6
};

// VLAN active state
enum VLANState
{
    VLAN_STATE_ACTIVE = 0x0000,
    VLAN_STATE_SUSPEND = 0x0001,
    MAX_VLAN_STATE
};

// Existing VLAN media types
enum VLANType
{
    VLAN_TYPE_NONE      = 0x0000, // not used by Cisco IOS
    VLAN_TYPE_ETHERNET  = 0x0001,
    VLAN_TYPE_FDDI      = 0x0002,
    VLAN_TYPE_TRCRF     = 0x0003, // token ring
    VLAN_TYPE_FDDI_NET  = 0x0004,
    VLAN_TYPE_TRBRF     = 0x0005,
    MAX_VLAN_TYPE
};

// STP modes
enum VLANSTPMode
{
    VLAN_STP_NONE       = 0x0000, // not used by Cisco IOS
    VLAN_STP_IEEE       = 0x0001,
    VLAN_STP_IBM        = 0x0002,
    MAX_VLAN_STP_MODE
};

// Bridge mode
enum VLANBridgeMode
{
    VLAN_BRMODE_NONE    = 0x0000, // not used by Cisco IOS
    VLAN_BRMODE_SRT     = 0x0001,
    VLAN_BRMODE_SRB     = 0x0002,
    MAX_VLAN_BRMODE
};

enum VLANBackupCrfMode
{
    VLAN_BCRF_MODE_NONE = 0x0000,
    VLAN_BCRF_MODE_OFF  = 0x0001,
    VLAN_BCRF_MODE_ON   = 0x0002,
    MAX_VLAN_BCRF_MODE
};

// VLAN features
enum VLANFeature
{
    VLAN_FEAT_NONE          = 0x00, // probably not used by Cisco IOS
    VLAN_FEAT_RING_NO       = 0x01,
    VLAN_FEAT_BRIDGE_NO     = 0x02,
    VLAN_FEAT_STP           = 0x03,
    VLAN_FEAT_PARENT        = 0x04,
    VLAN_FEAT_TRANS1        = 0x05,
    VLAN_FEAT_TRANS2        = 0x06,
    VLAN_FEAT_BRIDGE_MODE   = 0x07,
    VLAN_FEAT_ARE_HOPS      = 0x08,
    VLAN_FEAT_STE_HOPS      = 0x09,
    VLAN_FEAT_BACKUPCRF     = 0x0A,
    MAX_VLAN_FEATURE        = 0x0B, // no more known features
};

static const char* VLANFeatureNames[MAX_VLAN_FEATURE] = {
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

// domain name field length
#define MAX_VTP_DOMAIN_LENGTH 32

// length of timestamp field (summary advert)
#define VTP_TIMESTAMP_LENGTH 12
// length of MD5 hash (summary advert)
#define VTP_MD5_LENGTH 16

// for exact match, we need to eliminate additional padding
#pragma pack(push)
#pragma pack(1)

// since GCC6
//#pragma scalar_storage_order big-endian

struct VTPHeader
{
    uint8_t version;                            // VTP version (1, 2 or 3)
    uint8_t code;                               // packet type (see enum VTPMsgCode)
    union
    {
        uint8_t followers;                      // summary advert only - is this packet followed by subset advert packet?
        uint8_t sequence_nr;                    // subset advert only  - sequence number of packet, starting with 1
        uint8_t reserved;                       // advertisement request only - reserved, undefined value
    };
    uint8_t domain_len;                         // length of domain name (real)
    uint8_t domain_name[MAX_VTP_DOMAIN_LENGTH]; // domain name, padded with zeros to 32 bytes
};

struct SummaryAdvertPacketBody
{
    uint32_t revision;                              // revision of VLAN database
    uint32_t updater_id;                            // updater identity
    uint8_t update_timestamp[VTP_TIMESTAMP_LENGTH]; // timestamp of update
    uint8_t md5_digest[VTP_MD5_LENGTH];             // MD5 hash of VTP password (if set)
};

struct SubsetAdvertPacketBody
{
    uint32_t revision;                          // revision number
    uint8_t data;                               // data indicator; used just as base address member
};

struct AdvertRequestPacketBody
{
    uint32_t start_revision;                    // requested advertised revision
};

struct SubsetVLANInfoBody
{
    uint8_t length;                             // this portion length
    uint8_t status;                             // VLAN status
    uint8_t type;                               // VLAN type (see VLANType enum)
    uint8_t name_length;                        // length of vlan name
    uint16_t isl_vlan_id;                       // ISL VLAN ID
    uint16_t mtu_size;                          // MTU size
    uint32_t index80210;                        // 802.10 VLAN index
    uint8_t data;                               // data indicator; used just as base address member
};

// since GCC6
//#pragma scalar_storage_order default

#pragma pack(pop)

/**
 * Internal VLAN record structure
 */
struct VLANRecord
{
    // implicit constructor
    VLANRecord() { ResetRecordState(); };
    // full-parameter constructor
    VLANRecord(uint8_t _type, uint8_t _status, uint16_t _id, uint16_t _mtu, uint32_t _index80210, const char* _name) : type(_type), status(_status), id(_id), mtu(_mtu), index80210(_index80210), name(_name) { };

    void ResetRecordState() { id = 0; status = 0; type = 0; index80210 = 0; name = ""; mtu = 0; index80210 = 0; features.clear(); };

    // VLAN type
    uint8_t type;
    // VLAN status
    uint8_t status;
    // VLAN ID
    uint16_t id;
    // MTU size
    uint16_t mtu;
    // 802.10 index of VLAN
    uint32_t index80210;
    // VLAN name
    std::string name;
    // VLAN features
    std::map<uint16_t, uint16_t> features;
};

#endif
