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

#include "HubLocationCache.h"
#include "CreatureData.h"
#include "Log.h"
#include "ObjectMgr.h"
#include "UnitDefines.h"
#include <cmath>
#include <map>
#include <tuple>

namespace
{
// Same 50yd cell size as GrindLocationCache. AC's WANDER_NPC availability rule is ">= 3 allowed-flag
// NPCs within range", so a hub worth traveling to is a genuine cluster of >= 3 quest givers, not a
// lone wandering NPC (GrindLocationCache uses 2 for trash; hubs use 3 to match the mingle rule).
constexpr float HUB_CELL_SIZE = 50.0f;
constexpr uint32 HUB_CELL_MIN_SPAWNS = 3;

struct HubCellAccumulator
{
    double SumX = 0.0, SumY = 0.0, SumZ = 0.0;
    uint32 Count = 0;
};
}

HubLocationCache* HubLocationCache::instance()
{
    static HubLocationCache instance;
    return &instance;
}

std::vector<HubSpot> const& HubLocationCache::GetSpotsForMap(uint32 mapId)
{
    if (!_builtMaps.contains(mapId))
        BuildForMap(mapId);

    static std::vector<HubSpot> const empty;
    auto itr = _spotsByMap.find(mapId);
    return itr != _spotsByMap.end() ? itr->second : empty;
}

void HubLocationCache::BuildForMap(uint32 mapId)
{
    _builtMaps.insert(mapId);

    std::map<std::tuple<int32, int32, int32>, HubCellAccumulator> cells;

    for (auto const& [spawnId, data] : sObjectMgr->GetAllCreatureData())
    {
        if (data.mapId != mapId)
            continue;

        CreatureTemplate const* ct = sObjectMgr->GetCreatureTemplate(data.id);
        if (!ct)
            continue;

        // The anchor flag: a hub worth a trip is one that hands out quests. Honor the spawn-level
        // npcflag override if present (same uint64 field the live UNIT_NPC_FLAG_QUESTGIVER bit lives
        // in — schema-verified as a single npcflag on this Midnight core, no separate npcflag2 to
        // consult for questgiver), else the template default.
        uint64 const effectiveNpcFlag = data.npcflag.value_or(ct->npcflag);
        if (!(effectiveNpcFlag & UNIT_NPC_FLAG_QUESTGIVER))
            continue;

        int32 const cellX = int32(std::lround(data.spawnPoint.GetPositionX() / HUB_CELL_SIZE));
        int32 const cellY = int32(std::lround(data.spawnPoint.GetPositionY() / HUB_CELL_SIZE));
        int32 const cellZ = int32(std::lround(data.spawnPoint.GetPositionZ() / HUB_CELL_SIZE));

        HubCellAccumulator& acc = cells[{cellX, cellY, cellZ}];
        acc.SumX += data.spawnPoint.GetPositionX();
        acc.SumY += data.spawnPoint.GetPositionY();
        acc.SumZ += data.spawnPoint.GetPositionZ();
        ++acc.Count;
    }

    std::vector<HubSpot> spots;
    for (auto const& [key, acc] : cells)
    {
        if (acc.Count < HUB_CELL_MIN_SPAWNS)
            continue;

        HubSpot spot;
        spot.Pos.Relocate(float(acc.SumX / acc.Count), float(acc.SumY / acc.Count), float(acc.SumZ / acc.Count));
        spots.push_back(spot);
    }

    TC_LOG_DEBUG("playerbots", "HubLocationCache: built map {} with {} hub spots", mapId, uint32(spots.size()));

    _spotsByMap[mapId] = std::move(spots);
}
