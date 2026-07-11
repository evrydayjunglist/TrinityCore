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
#include "DatabaseEnv.h"
#include "DB2Stores.h"
#include "Log.h"
#include "Timer.h"
#include <algorithm>

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

void ArchaeologyMgr::LoadDigSiteData()
{
    uint32 oldMSTime = getMSTime();

    _digSiteInfo.clear();

    //                                               0                 1                 2
    QueryResult result = WorldDatabase.Query("SELECT researchSiteId, researchBranchId, findCount FROM archaeology_dig_site");
    if (!result)
    {
        TC_LOG_INFO("server.loading", ">> Loaded 0 archaeology dig-site branch mappings. DB table `archaeology_dig_site` is empty.");
        return;
    }

    uint32 count = 0;
    do
    {
        Field* fields = result->Fetch();
        uint32 siteId = fields[0].GetUInt32();

        if (!sResearchSiteStore.HasRecord(siteId))
        {
            TC_LOG_ERROR("sql.sql", "Table `archaeology_dig_site` has researchSiteId {} with no matching ResearchSite.db2 entry, skipped.", siteId);
            continue;
        }

        ArchaeologyDigSiteInfo& info = _digSiteInfo[siteId];
        info.BranchID = fields[1].GetUInt32();
        info.FindCount = fields[2].GetUInt8();
        ++count;
    } while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded {} archaeology dig-site branch mappings in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
}

void ArchaeologyMgr::LoadDigSitePoints()
{
    uint32 oldMSTime = getMSTime();

    //                                               0               1     2
    QueryResult result = WorldDatabase.Query("SELECT researchSiteId, posX, posY FROM archaeology_dig_site_point ORDER BY researchSiteId, idx");
    if (!result)
    {
        TC_LOG_INFO("server.loading", ">> Loaded 0 archaeology dig-site polygons. DB table `archaeology_dig_site_point` is empty.");
        return;
    }

    uint32 points = 0, sites = 0;
    do
    {
        Field* fields = result->Fetch();
        uint32 siteId = fields[0].GetUInt32();

        auto itr = _digSiteInfo.find(siteId);
        if (itr == _digSiteInfo.end())
        {
            TC_LOG_ERROR("sql.sql", "Table `archaeology_dig_site_point` has researchSiteId {} with no branch mapping in `archaeology_dig_site`, skipped.", siteId);
            continue;
        }

        if (itr->second.Polygon.empty())
            ++sites;

        itr->second.Polygon.emplace_back(fields[1].GetFloat(), fields[2].GetFloat());
        ++points;
    } while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded {} archaeology dig-site polygons ({} points) in {} ms", sites, points, GetMSTimeDiffToNow(oldMSTime));
}

ArchaeologyDigSiteInfo const* ArchaeologyMgr::GetDigSiteInfo(uint32 researchSiteId) const
{
    auto itr = _digSiteInfo.find(researchSiteId);
    return itr != _digSiteInfo.end() ? &itr->second : nullptr;
}

bool ArchaeologyMgr::IsInsideDigSite(uint32 researchSiteId, float x, float y) const
{
    ArchaeologyDigSiteInfo const* info = GetDigSiteInfo(researchSiteId);
    if (!info || info->Polygon.size() < 3)
        return false;

    // Standard ray-casting point-in-polygon test on the world X/Y boundary.
    std::vector<std::pair<float, float>> const& poly = info->Polygon;
    bool inside = false;
    for (std::size_t i = 0, j = poly.size() - 1; i < poly.size(); j = i++)
    {
        float xi = poly[i].first, yi = poly[i].second;
        float xj = poly[j].first, yj = poly[j].second;
        if (((yi > y) != (yj > y)) && (x < (xj - xi) * (y - yi) / (yj - yi) + xi))
            inside = !inside;
    }
    return inside;
}

std::vector<uint32> ArchaeologyMgr::RollResearchSitesForMap(uint32 mapId, uint32 count) const
{
    std::vector<uint32> result;

    std::vector<ResearchSiteEntry const*> const* pool = GetResearchSitesForMap(mapId);
    if (!pool || pool->empty() || !count)
        return result;

    std::vector<ResearchSiteEntry const*> picks = *pool;
    // Only assign sites we can fully drive (branch mapping + boundary polygon), so every active site
    // the player receives is surveyable. Unmapped sites (e.g. Archy gaps) are skipped for now.
    picks.erase(std::remove_if(picks.begin(), picks.end(),
        [this](ResearchSiteEntry const* site) { return !GetDigSiteInfo(site->ID); }), picks.end());

    if (picks.size() > count)
        Trinity::Containers::RandomResize(picks, count);

    result.reserve(picks.size());
    for (ResearchSiteEntry const* site : picks)
        result.push_back(site->ID);

    return result;
}
