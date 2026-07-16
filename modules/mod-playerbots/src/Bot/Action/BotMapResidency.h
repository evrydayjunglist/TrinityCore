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

#ifndef TRINITY_PLAYERBOT_BOT_MAP_RESIDENCY_H
#define TRINITY_PLAYERBOT_BOT_MAP_RESIDENCY_H

#include "Map.h"
#include "Player.h"
#include "Position.h"

// Shared residency gate for bot AI on the map-update thread (freeze classes 1 + 3).
//
// "Would a terrain/liquid/area query — or a MovePoint leg — at this position force a
// synchronous grid+VMAP(+MMAP) disk load inside Map::Update?" Bot AI ticks on MapUpdater
// workers, so Map::GetZoneId/GetHeight on a cold position (class 1) or walking a visibility
// frontier into unloaded neighbour grids (class 3 DelayedUnitRelocation → EnsureGridLoaded)
// can serialise past MaxCoreStuckTime and trip FreezeDetector.
//
// A created NGrid eagerly loads terrain+VMAP+MMAP (Map::EnsureGridCreated), so
// IsGridLoaded(pos)==true ⇒ subsequent queries and short travel legs are memory reads.
// Public Map API only — TerrainInfo::_loadedGrids is private. AC precedent:
// TravelMgr GuidPosition::IsCreatureOrGOAccessible gates on map->IsGridLoaded(...).
//
// Rules: the bot's own resident grid is always fine. For any remote destination or path
// endpoint, gate first — skip / re-roll / idle; never force the load from AI.
inline bool IsBotMapPosQueryable(Player const* bot, Position const& pos)
{
    return bot && bot->IsInWorld() && bot->GetMap()->IsGridLoaded(pos);
}

inline bool IsBotMapPosQueryable(Player const* bot, float x, float y)
{
    return bot && bot->IsInWorld() && bot->GetMap()->IsGridLoaded(x, y);
}

#endif
