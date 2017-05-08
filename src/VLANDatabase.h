/********************************
 * KIV/PSI: VTP node            *
 * Semestral work               *
 *                              *
 * Author: Martin UBL           *
 *         A16N0026P            *
 *         ublm@students.zcu.cz *
 ********************************/

#ifndef VLAN_DATABASE_H
#define VLAN_DATABASE_H

#include "Singleton.h"
#include "VTPDefs.h"

typedef std::map<uint16_t, VLANRecord*> VLANMap;

/*
 * VLAN database structure - used to store VLAN data
 */
class VLANDatabaseService : public CSingleton<VLANDatabaseService>
{
    private:
        VLANMap m_vlans;

    protected:
        // reduces stored VLANs to supplied set only
        void _ReduceVLANSet(std::set<uint16_t> const &vlanSet);

    public:
        VLANDatabaseService();

        // finds and returns VLAN by its name
        VLANRecord* GetVLANByName(const char* name);
        // finds and returns VLAN by its ID
        VLANRecord* GetVLANById(uint16_t id);
        // adds new VLAN to domain
        void AddVLAN(uint16_t id, uint8_t type, uint8_t status, uint16_t mtu, uint32_t index80210, const char* name);
        // updates existing VLAN in domain
        void UpdateVLAN(uint16_t id, uint8_t type, uint8_t status, uint16_t mtu, uint32_t index80210, const char* name);
        // removes VLAN from domain
        void RemoveVLAN(uint16_t id);
        // flushes VLAN database after manipulations; vlanSet contains set of VLANs to be preserved
        void Flush(std::set<uint16_t> const &vlanSet);

        // retrieves all VLAN map
        VLANMap const& GetVLANMap() const;
};

#define sVLANDatabase VLANDatabaseService::getInstance()

#endif
