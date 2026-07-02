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

#include "GrindAction.h"
#include "AttackValidity.h"
#include "Bot/Rpg/GrindLocationCache.h"
#include "CellImpl.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "MotionMaster.h"
#include "Player.h"
#include "Unit.h"

namespace
{
constexpr float GRIND_TARGET_SEARCH_RADIUS = 30.0f;
constexpr float GRIND_SPOT_MAX_DISTANCE = 250.0f;
constexpr uint32 GRIND_LEVEL_TOLERANCE = 5;

Unit* FindNearbyHostile(Player* bot)
{
    Unit* victim = nullptr;
    Trinity::NearestAttackableUnitInObjectRangeCheck check(bot, bot, GRIND_TARGET_SEARCH_RADIUS);
    Trinity::UnitLastSearcher<Trinity::NearestAttackableUnitInObjectRangeCheck> searcher(bot, victim, check);
    Cell::VisitAllObjects(bot, searcher, GRIND_TARGET_SEARCH_RADIUS);

    if (victim && IsValidAttackTarget(bot, victim))
        return victim;

    return nullptr;
}

bool IsSpotForBotLevel(Player* bot, GrindSpot const& spot)
{
    // Scaling-aware spots (ContentTuningID != 0) are viable for any bot level in range —
    // don't force them into a static AC-style level bucket (handoff § TC-Midnight adaptations).
    if (spot.ScalingAware)
        return true;

    uint32 const level = bot->GetLevel();
    return level + GRIND_LEVEL_TOLERANCE >= spot.MinLevel && level <= spot.MaxLevel + GRIND_LEVEL_TOLERANCE;
}

GrindSpot const* FindGrindDestination(Player* bot)
{
    std::vector<GrindSpot> const& spots = sGrindLocationCache->GetSpotsForMap(bot->GetMapId());
    if (spots.empty())
        return nullptr;

    GrindSpot const* best = nullptr;
    float bestDistSq = GRIND_SPOT_MAX_DISTANCE * GRIND_SPOT_MAX_DISTANCE;

    for (GrindSpot const& spot : spots)
    {
        if (!IsSpotForBotLevel(bot, spot))
            continue;

        float const distSq = bot->GetExactDistSq(spot.Pos);
        if (distSq < bestDistSq)
        {
            bestDistSq = distSq;
            best = &spot;
        }
    }

    return best;
}
}

bool GrindAction::IsUseful()
{
    Player* bot = GetBot();
    if (!bot || !bot->IsInWorld() || bot->IsInCombat())
        return false;

    if (FindNearbyHostile(bot))
        return true;

    return FindGrindDestination(bot) != nullptr;
}

bool GrindAction::Execute(Event /*event*/)
{
    Player* bot = GetBot();
    if (!bot)
        return false;

    if (Unit* target = FindNearbyHostile(bot))
    {
        bot->SetSelection(target->GetGUID());
        if (!bot->HasInArc(float(M_PI), target))
            bot->SetFacingToObject(target);
        return bot->Attack(target, true);
    }

    GrindSpot const* spot = FindGrindDestination(bot);
    if (!spot)
        return false;

    if (bot->GetExactDist(spot->Pos) <= GRIND_TARGET_SEARCH_RADIUS)
        return false; // already there — nothing hostile right now, let wander/quest-giver take over

    if (bot->GetMotionMaster()->GetCurrentMovementGeneratorType() == POINT_MOTION_TYPE)
        return false; // already walking toward a destination this tick

    bot->GetMotionMaster()->MovePoint(0, spot->Pos.GetPositionX(), spot->Pos.GetPositionY(), spot->Pos.GetPositionZ());
    return true;
}
