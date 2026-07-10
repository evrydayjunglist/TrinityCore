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

#ifndef TRINITY_PLAYERBOT_HUB_LOCATION_CACHE_H
#define TRINITY_PLAYERBOT_HUB_LOCATION_CACHE_H

#include "Position.h"
#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include <vector>

// RPG hub-travel layer ("where to go get quests") — the GO_CAMP counterpart of Gate 10's
// GrindLocationCache, and built exactly the same way (lazily per map, one instance for the whole
// module, iterating sObjectMgr->GetAllCreatureData() rather than a separate DB round-trip). The
// only difference from the grind cache is the filter: instead of clustering hostile trash mobs it
// clusters service-NPC spawns — creatures flagged UNIT_NPC_FLAG_QUESTGIVER — so a cell that holds a
// genuine town/quest hub (>= HUB_CELL_MIN_SPAWNS givers) surfaces as a travel destination. A bot
// stranded in a giver-less pocket rolls RPG_GO_CAMP, travels to the nearest such centroid, and then
// mingles there via RPG_WANDER_NPC. AC reference: NewRpgBaseAction::SelectRandomCampPos over
// TravelMgr.GetTravelHubs(); this is the TC-native source for those hubs.
// See playerbots-rpg-hub-travel-lifelike-wander-rest-handoff.md §3.
struct HubSpot
{
    Position Pos;
};

class HubLocationCache
{
public:
    static HubLocationCache* instance();

    // Builds the cache for this map on first call (world-restart lifetime after that — the same
    // lazy-staleness contract as GrindLocationCache; downstream GO_CAMP travel/arrival always
    // re-validates against the live world, so a stale centroid is harmless).
    std::vector<HubSpot> const& GetSpotsForMap(uint32 mapId);

private:
    HubLocationCache() = default;

    void BuildForMap(uint32 mapId);

    // Bot AI ticks inside Map::Update on parallel MapUpdater worker threads, and this cache is a
    // single module-wide instance shared across every map. Without synchronisation, two maps being
    // first-built concurrently would race on the containers below (unsynchronised std::unordered_map
    // insert/rehash is undefined behaviour — a latent freeze/corruption vector). All access to
    // _spotsByMap/_builtMaps goes through this mutex; the build is once-per-map, so after warmup the
    // lock is uncontended and the per-tick cost is a negligible lock around a find().
    std::mutex _mutex;
    std::unordered_map<uint32, std::vector<HubSpot>> _spotsByMap;
    std::unordered_set<uint32> _builtMaps;
};

#define sHubLocationCache HubLocationCache::instance()

#endif
