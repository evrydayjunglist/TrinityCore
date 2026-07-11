/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "ArchaeologyMgr.h"
#include "Containers.h"
#include "DB2Stores.h"
#include "Log.h"
#include "Timer.h"

ArchaeologyMgr::ArchaeologyMgr() = default;
ArchaeologyMgr::~ArchaeologyMgr() = default;

ArchaeologyMgr* ArchaeologyMgr::instance()
{
    static ArchaeologyMgr instance;
    return &instance;
}

void ArchaeologyMgr::LoadResearchSites()
{
    uint32 oldMSTime = getMSTime();

    _researchSitesByMap.clear();

    uint32 count = 0;
    for (ResearchSiteEntry const* site : sResearchSiteStore)
    {
        if (site->MapID < 0)
            continue;

        _researchSitesByMap[uint32(site->MapID)].push_back(site);
        ++count;
    }

    TC_LOG_INFO("server.loading", ">> Loaded {} archaeology research sites across {} maps in {} ms",
        count, _researchSitesByMap.size(), GetMSTimeDiffToNow(oldMSTime));
}

std::vector<ResearchSiteEntry const*> const* ArchaeologyMgr::GetResearchSitesForMap(uint32 mapId) const
{
    auto itr = _researchSitesByMap.find(mapId);
    return itr != _researchSitesByMap.end() ? &itr->second : nullptr;
}

std::vector<uint32> ArchaeologyMgr::RollResearchSitesForMap(uint32 mapId, uint32 count) const
{
    std::vector<uint32> result;

    std::vector<ResearchSiteEntry const*> const* pool = GetResearchSitesForMap(mapId);
    if (!pool || pool->empty() || !count)
        return result;

    std::vector<ResearchSiteEntry const*> picks = *pool;
    if (picks.size() > count)
        Trinity::Containers::RandomResize(picks, count);

    result.reserve(picks.size());
    for (ResearchSiteEntry const* site : picks)
        result.push_back(site->ID);

    return result;
}
