#include "general.h"
#include "VLANDatabase.h"

#include "VTPDefs.h"

VLANDatabaseService::VLANDatabaseService()
{
    //
}


VLANRecord* VLANDatabaseService::GetVLANByName(const char* name)
{
    for (auto it : m_vlans)
    {
        if (it.second->name == name)
            return it.second;
    }

    return nullptr;
}

VLANRecord* VLANDatabaseService::GetVLANById(uint16_t id)
{
    auto it = m_vlans.find(id);
    if (it == m_vlans.end())
        return nullptr;

    return it->second;
}

void VLANDatabaseService::AddVLAN(uint16_t id, uint8_t type, uint8_t status, uint16_t mtu, uint32_t index80210, const char* name)
{
    if (GetVLANById(id) || GetVLANByName(name))
        return;

    m_vlans[id] = new VLANRecord(type, status, id, mtu, index80210, name);
}

void VLANDatabaseService::UpdateVLAN(uint16_t id, uint8_t type, uint8_t status, uint16_t mtu, uint32_t index80210, const char* name)
{
    VLANRecord* vlan = GetVLANById(id);
    if (!vlan)
        return;

    vlan->type = type;
    vlan->status = status;
    vlan->index80210 = index80210;
    vlan->mtu = mtu;
    vlan->name = name;
}

void VLANDatabaseService::RemoveVLAN(uint16_t id)
{
    delete m_vlans[id];
    m_vlans.erase(id);
}

VLANMap const& VLANDatabaseService::GetVLANMap() const
{
    return m_vlans;
}

void VLANDatabaseService::_ReduceVLANSet(std::set<uint16_t> const &vlanSet)
{
    std::set<uint16_t> toRemove;

    // find VLANs to be removed
    for (auto it : m_vlans)
    {
        if (vlanSet.find(it.first) == vlanSet.end())
            toRemove.insert(it.first);
    }

    for (uint16_t vlanId : toRemove)
        RemoveVLAN(vlanId);
}

void VLANDatabaseService::Flush(std::set<uint16_t> const &vlanSet)
{
    _ReduceVLANSet(vlanSet);
}
