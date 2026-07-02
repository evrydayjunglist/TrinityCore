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

#include "GrindLocationCache.h"
#include "CreatureData.h"
#include "DB2Stores.h"
#include "DBCEnums.h"
#include "Log.h"
#include "ObjectMgr.h"
#include "SharedDefines.h"
#include "UnitDefines.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <map>
#include <tuple>

namespace
{
constexpr float GRIND_CELL_SIZE = 50.0f;
constexpr uint32 GRIND_CELL_MIN_SPAWNS = 2;

bool IsViableGrindTemplate(CreatureTemplate const* ct, CreatureDifficulty const* diff)
{
    if (!ct || !diff)
        return false;

    // Not a vendor/quest-giver/trainer/etc — a "plain" mob, matching AC's npcflag == 0 filter.
    if (ct->npcflag != 0)
        return false;

    // Only trash-tier creatures are auto-grind fodder, matching AC's rank == 0 filter —
    // elites/rares/bosses are questing/grouping content, not solo-grind targets.
    if (ct->Classification != CreatureClassifications::Normal)
        return false;

    FactionTemplateEntry const* faction = sFactionTemplateStore.LookupEntry(ct->faction);
    if (!faction || !faction->IsHostileToPlayers())
        return false;

    // No loot table => not worth grinding; also filters out decorative/critter templates,
    // matching AC's lootid != 0 filter.
    if (diff->LootID == 0)
        return false;

    return true;
}

// Exclusion + resolution rule from the Gate 10 pre-implementation evidence check
// (2026-07-02, see handoff § Pre-implementation evidence check): ContentTuningID == 0 AND
// both deltas == 0 means unset/placeholder level data, not a real grind target.
bool TryResolveLevelRange(CreatureDifficulty const* diff, bool& scalingAware, uint32& minLevel, uint32& maxLevel)
{
    if (diff->ContentTuningID == 0 && diff->DeltaLevelMin == 0 && diff->DeltaLevelMax == 0)
        return false;

    if (diff->ContentTuningID != 0)
    {
        Optional<ContentTuningLevels> levels = sDB2Manager.GetContentTuningData(diff->ContentTuningID, {});
        if (!levels)
            return false;

        scalingAware = true;
        minLevel = uint32(std::max<int16>(levels->MinLevel, 1));
        maxLevel = uint32(std::max<int16>(levels->MaxLevel, int16(minLevel)));
        return true;
    }

    // Non-scaling fallback: no absolute minlevel/maxlevel column exists on this schema
    // (see handoff § TC-Midnight adaptations), so best-effort bucket by the delta values
    // themselves. Expected to be rare in practice — the evidence check found nearly every
    // real creature already carries a ContentTuningID in this Midnight world DB.
    scalingAware = false;
    int32 const lo = std::min(diff->DeltaLevelMin, diff->DeltaLevelMax);
    int32 const hi = std::max(diff->DeltaLevelMin, diff->DeltaLevelMax);
    minLevel = uint32(std::max<int32>(std::abs(lo), 1));
    maxLevel = uint32(std::max<int32>(std::abs(hi), int32(minLevel)));
    return true;
}

struct GrindCellAccumulator
{
    double SumX = 0.0, SumY = 0.0, SumZ = 0.0;
    uint32 Count = 0;
    bool ScalingAware = false;
    uint32 MinLevel = std::numeric_limits<uint32>::max();
    uint32 MaxLevel = 0;
};
}

GrindLocationCache* GrindLocationCache::instance()
{
    static GrindLocationCache instance;
    return &instance;
}

std::vector<GrindSpot> const& GrindLocationCache::GetSpotsForMap(uint32 mapId)
{
    if (!_builtMaps.contains(mapId))
        BuildForMap(mapId);

    static std::vector<GrindSpot> const empty;
    auto itr = _spotsByMap.find(mapId);
    return itr != _spotsByMap.end() ? itr->second : empty;
}

void GrindLocationCache::BuildForMap(uint32 mapId)
{
    _builtMaps.insert(mapId);

    std::map<std::tuple<int32, int32, int32>, GrindCellAccumulator> cells;

    for (auto const& [spawnId, data] : sObjectMgr->GetAllCreatureData())
    {
        if (data.mapId != mapId)
            continue;

        CreatureTemplate const* ct = sObjectMgr->GetCreatureTemplate(data.id);
        if (!ct)
            continue;

        uint32 const effectiveUnitFlags = data.unit_flags.value_or(ct->unit_flags);
        if (effectiveUnitFlags & (UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_NON_ATTACKABLE))
            continue;

        CreatureDifficulty const* diff = ct->GetDifficulty(DIFFICULTY_NONE);
        if (!IsViableGrindTemplate(ct, diff))
            continue;

        bool scalingAware = false;
        uint32 minLevel = 0, maxLevel = 0;
        if (!TryResolveLevelRange(diff, scalingAware, minLevel, maxLevel))
            continue;

        int32 const cellX = int32(std::lround(data.spawnPoint.GetPositionX() / GRIND_CELL_SIZE));
        int32 const cellY = int32(std::lround(data.spawnPoint.GetPositionY() / GRIND_CELL_SIZE));
        int32 const cellZ = int32(std::lround(data.spawnPoint.GetPositionZ() / GRIND_CELL_SIZE));

        GrindCellAccumulator& acc = cells[{cellX, cellY, cellZ}];
        acc.SumX += data.spawnPoint.GetPositionX();
        acc.SumY += data.spawnPoint.GetPositionY();
        acc.SumZ += data.spawnPoint.GetPositionZ();
        ++acc.Count;
        acc.ScalingAware = acc.ScalingAware || scalingAware;
        acc.MinLevel = std::min(acc.MinLevel, minLevel);
        acc.MaxLevel = std::max(acc.MaxLevel, maxLevel);
    }

    std::vector<GrindSpot> spots;
    for (auto const& [key, acc] : cells)
    {
        if (acc.Count < GRIND_CELL_MIN_SPAWNS)
            continue;

        GrindSpot spot;
        spot.Pos.Relocate(float(acc.SumX / acc.Count), float(acc.SumY / acc.Count), float(acc.SumZ / acc.Count));
        spot.ScalingAware = acc.ScalingAware;
        spot.MinLevel = acc.MinLevel;
        spot.MaxLevel = acc.MaxLevel;
        spots.push_back(spot);
    }

    TC_LOG_DEBUG("playerbots", "GrindLocationCache: built map {} with {} grind spots", mapId, uint32(spots.size()));

    _spotsByMap[mapId] = std::move(spots);
}
