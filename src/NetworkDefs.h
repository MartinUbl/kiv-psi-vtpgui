/********************************
 * KIV/PSI: VTP node            *
 * Semestral work               *
 *                              *
 * Author: Martin UBL           *
 *         A16N0026P            *
 *         ublm@students.zcu.cz *
 ********************************/

#ifndef NETWORK_DEFS_H
#define NETWORK_DEFS_H

// MAC address length
#define MAC_ADDR_LENGTH 6
// IP address length
#define IP_ADDR_LENGTH 4
// length of SNAP OUI field
#define SNAP_OUI_LEN 3

#pragma pack(push)
#pragma pack(1)

// main headers (802.3/EthernetII, 802.1Q)

struct EthernetHeader
{
    uint8_t dest_mac[MAC_ADDR_LENGTH];          // destination MAC address
    uint8_t src_mac[MAC_ADDR_LENGTH];           // source MAC address
    union
    {
        uint16_t frame_length;                  // length (802.3 Ethernet)
        uint16_t ether_type;                    // etherType (Ethernet II)
    };
};

struct Dot1QHeader
{
    uint8_t dest_mac[MAC_ADDR_LENGTH];          // destination MAC address
    uint8_t src_mac[MAC_ADDR_LENGTH];           // source MAC address
    uint16_t tpid;                              // TPID (Tag Protocol Identifier), 0x8100 for 802.1Q tagged encapsulation
    uint16_t tci;                               // TCI (Tag Control Information), PCP (3b), DEI (1b), VLAN ID (12b)
    union
    {
        uint16_t frame_length;                  // length (802.3 Ethernet)
        uint16_t ether_type;                    // etherType (Ethernet II)
    };
};

// protocol extensions, LLC headers, 802.2

struct LLCHeader
{
    uint8_t dsap;                               // DSAP field (0xAA for SNAP)
    uint8_t ssap;                               // SSAP field (0xAA for SNAP)
    uint8_t control;                            // control field (may be 2 bytes, we use just 1 byte version here), (0x03 for 802.2 PDUs)
};

struct SNAPHeader
{
    uint8_t oui[SNAP_OUI_LEN];                  // organization identifier (0x00000C for Cisco)
    uint16_t protocol_id;                       // encapsulated protocol ID (0x2003 for VTP)
};

#pragma pack(pop)

#endif
