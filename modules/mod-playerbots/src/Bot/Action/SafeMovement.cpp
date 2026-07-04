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

#include "SafeMovement.h"
#include "Map.h"
#include "MotionMaster.h"
#include "PathGenerator.h"
#include "Player.h"
#include "PlayerbotsConfig.h"
#include <cmath>
#include <cstddef>

namespace
{
// Same acceptance set AC mod-playerbots uses in NewRpgBaseAction::MoveRandomNear/MoveFarTo:
// a real (possibly partial) navmesh route, or a start/end point slightly off the mmap polygon.
// Deliberately excludes PATHFIND_SHORTCUT ("travel through obstacles, terrain, air, etc — old
// behavior") and PATHFIND_NOPATH — PATHFIND_SHORTCUT is exactly the straight-line-through-terrain
// fallback that clips a bot through a steep incline instead of following the ground.
constexpr uint32 PATH_TYPE_ACCEPTABLE = PATHFIND_NORMAL | PATHFIND_INCOMPLETE | PATHFIND_FARFROMPOLY;

// Segments shorter than this (horizontal distance, yards) are treated as a step/ledge — recast's
// own walkableClimb already governs those — not an incline. Without this floor, a legitimate small
// step up (large dz over a tiny dx/dy) would read as a near-vertical "wall" and get rejected.
constexpr float MIN_HORIZONTAL_FOR_SLOPE_CHECK = 1.0f;

// Fork-native defense-in-depth check, not an AC port — see playerbots-bot-wander-ground-clip-
// handoff.md §5. PathGenerator already confines a Player-type source (bots are Players) to
// NAV_GROUND (<=55 degree) polygons and excludes the 55-70 degree NAV_GROUND_STEEP area reserved
// for combat creatures (PathGenerator::CreateFilter), so this never fires on genuinely disconnected
// terrain — CalculatePath would already return PATHFIND_SHORTCUT/NOPATH for that, caught above.
// What it catches instead: a technically-connected, technically <=55 degree route whose real-world
// incline still reads as "walking up a wall" to a human watching (coarse collision-mesh triangles
// can average out a locally steeper visual slope). Walks every corridor waypoint pair the bot is
// actually about to be sent along and refuses the whole move if any one segment's rise-over-run
// angle exceeds the configured ceiling.
bool HasWalkableSlope(Movement::PointsArray const& points)
{
    float const maxSlopeDeg = Playerbots::GetMaxWalkableSlopeDegrees();
    for (std::size_t i = 1; i < points.size(); ++i)
    {
        G3D::Vector3 const& prev = points[i - 1];
        G3D::Vector3 const& next = points[i];
        float const dx = next.x - prev.x;
        float const dy = next.y - prev.y;
        float const horizontal = std::sqrt(dx * dx + dy * dy);
        if (horizontal < MIN_HORIZONTAL_FOR_SLOPE_CHECK)
            continue; // step/ledge, not an incline — leave it to recast's walkableClimb

        float const dz = std::fabs(next.z - prev.z);
        float const angleDeg = std::atan2(dz, horizontal) * (180.0f / float(M_PI));
        if (angleDeg > maxSlopeDeg)
            return false;
    }
    return true;
}
}

namespace
{
// Shared path/slope gate for every entry point below: reject a disconnected/shortcut route or
// one whose corridor is steeper than the configured ceiling. Returns the validated (possibly
// partial-route) end position on success so callers that also commit a MovePoint don't have to
// recompute the path a second time.
bool ValidatePathAndSlope(Player* bot, float x, float y, float z, PathGenerator& outPath)
{
    if (!bot)
        return false;

    outPath.CalculatePath(x, y, z);
    if (outPath.GetPathType() & ~PATH_TYPE_ACCEPTABLE)
        return false; // no real mmap route — refuse rather than walk straight through terrain

    if (!HasWalkableSlope(outPath.GetPath()))
        return false; // technically-connected but too steep to be a real player-climbable incline

    return true;
}

// Shared body for both one-shot MovePoint entry points: full contract (acceptable path type,
// per-segment slope, no-water endpoint), plus an optional minimum-progress requirement used by
// far travel — pathological PATHFIND_INCOMPLETE results can put the reachable endpoint right
// under the bot's feet, and committing that "move" would just burn the tick without closing any
// gap.
bool TryMoveValidated(Player* bot, float x, float y, float z, float minProgress)
{
    if (!bot)
        return false;

    PathGenerator path(bot);
    if (!ValidatePathAndSlope(bot, x, y, z, path))
        return false;

    G3D::Vector3 const& end = path.GetActualEndPosition();
    if (bot->GetMap()->IsInWater(bot->GetPhaseShift(), end.x, end.y, end.z))
        return false; // wander/grind/approach targets should never route a bot into water

    if (minProgress > 0.0f)
    {
        float const distNow = bot->GetExactDist(x, y, z);
        float const distAfter = std::sqrt((x - end.x) * (x - end.x) + (y - end.y) * (y - end.y) + (z - end.z) * (z - end.z));
        if (distAfter + minProgress > distNow)
            return false; // partial-route endpoint doesn't actually close the gap — let the caller sample elsewhere
    }

    bot->GetMotionMaster()->MovePoint(0, end.x, end.y, end.z);
    return true;
}
}

bool TryMoveToValidatedPoint(Player* bot, float x, float y, float z)
{
    return TryMoveValidated(bot, x, y, z, 0.0f);
}

bool TryMoveTowardValidatedPoint(Player* bot, float x, float y, float z, float minProgress)
{
    return TryMoveValidated(bot, x, y, z, minProgress);
}

bool IsApproachPathWalkable(Player* bot, float x, float y, float z)
{
    if (!bot)
        return false;

    PathGenerator path(bot);
    return ValidatePathAndSlope(bot, x, y, z, path);
}
