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

#ifndef TRINITY_PLAYERBOT_GRIND_LOCATION_CACHE_H
#define TRINITY_PLAYERBOT_GRIND_LOCATION_CACHE_H

#include "Position.h"
#include <unordered_map>
#include <unordered_set>
#include <vector>

// Gate 10, Layer 1 ("where to grind") — lazily built per map, one instance for the whole
// module (not per-bot). AC reference: mod-playerbots-master/src/Mgr/Travel/TravelMgr.cpp
// PrepareDestinationCache(), which itself iterates sObjectMgr->GetAllCreatureData() rather
// than issuing its own SQL query — this cache does the same on TC, so no separate DB
// round-trip is needed; see playerbots-gate-10-world-rpg-slice-handoff.md § Phase B.
struct GrindSpot
{
    Position Pos;
    bool ScalingAware = false;
    uint32 MinLevel = 0;
    uint32 MaxLevel = 0;
};

class GrindLocationCache
{
public:
    static GrindLocationCache* instance();

    // Builds the cache for this map on first call (world-restart lifetime after that —
    // Layer 2's live search always re-validates targets, so a stale cell is harmless).
    std::vector<GrindSpot> const& GetSpotsForMap(uint32 mapId);

private:
    GrindLocationCache() = default;

    void BuildForMap(uint32 mapId);

    std::unordered_map<uint32, std::vector<GrindSpot>> _spotsByMap;
    std::unordered_set<uint32> _builtMaps;
};

#define sGrindLocationCache GrindLocationCache::instance()

#endif
