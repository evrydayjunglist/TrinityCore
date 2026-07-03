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

#include "WanderAction.h"
#include "MotionMaster.h"
#include "Player.h"
#include "Random.h"
#include "SafeMovement.h"
#include <cmath>

namespace
{
constexpr float WANDER_MIN_RADIUS = 10.0f;
constexpr float WANDER_MAX_RADIUS = 30.0f;

// AC mod-playerbots' NewRpgBaseAction::MoveRandomNear retries up to 8 random samples before
// giving up — a single bad roll (cliff edge, water, disconnected navmesh poly) shouldn't leave
// the bot stuck standing still forever. Matched here rather than picking an arbitrary count.
constexpr int WANDER_MAX_ATTEMPTS = 8;
}

bool WanderAction::IsUseful()
{
    Player* bot = GetBot();
    if (!bot || !bot->IsInWorld() || bot->IsInCombat())
        return false;

    // Only wander when idle — avoid overriding movement already in progress (grind/quest-giver
    // approach, or a previous wander leg still playing out).
    return bot->GetMotionMaster()->GetCurrentMovementGeneratorType() == IDLE_MOTION_TYPE;
}

bool WanderAction::Execute(Event /*event*/)
{
    Player* bot = GetBot();
    if (!bot)
        return false;

    float const x = bot->GetPositionX(), y = bot->GetPositionY(), z = bot->GetPositionZ();

    // Sample random points around the bot's current position and validate each one via a real
    // mmap path before committing (see SafeMovement.h) — a raw random-point-then-height-snap (the
    // previous approach here) can land on the wrong side of a steep slope and walk the bot
    // straight through the terrain to get there. Retry on rejection instead of moving anyway.
    for (int attempt = 0; attempt < WANDER_MAX_ATTEMPTS; ++attempt)
    {
        float const distance = frand(WANDER_MIN_RADIUS, WANDER_MAX_RADIUS);
        float const angle = frand(0.0f, 2.0f * float(M_PI));
        float const dx = x + distance * std::cos(angle);
        float const dy = y + distance * std::sin(angle);

        if (TryMoveToValidatedPoint(bot, dx, dy, z))
            return true;
    }

    return false; // nothing reachable nearby this tick — stand still rather than clip through terrain
}
