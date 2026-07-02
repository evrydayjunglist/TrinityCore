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

namespace
{
constexpr float WANDER_MIN_RADIUS = 10.0f;
constexpr float WANDER_MAX_RADIUS = 30.0f;
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

    Position dest = bot->GetRandomPoint(bot->GetPosition(), WANDER_MAX_RADIUS, WANDER_MIN_RADIUS);
    float x = dest.GetPositionX(), y = dest.GetPositionY(), z = dest.GetPositionZ();
    bot->UpdateAllowedPositionZ(x, y, z);

    bot->GetMotionMaster()->MovePoint(0, x, y, z);
    return true;
}
