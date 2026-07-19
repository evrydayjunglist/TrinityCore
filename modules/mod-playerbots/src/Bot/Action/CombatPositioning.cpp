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

#include "CombatPositioning.h"
#include "BotMapResidency.h"
#include "DBCEnums.h"
#include "DB2Structure.h"
#include "MotionMaster.h"
#include "MovementDefines.h"
#include "Player.h"
#include "PlayerbotsConfig.h"
#include "SafeMovement.h"
#include "Unit.h"

#include <cmath>

namespace
{
constexpr float RANGED_BAND_INNER_FACTOR = 0.70f;
constexpr float RANGED_BAND_OUTER_FACTOR = 1.15f;
constexpr float RANGED_BACKPEDAL_STEP = 8.0f;

bool TryBackpedalFromTarget(Player* bot, Unit* target)
{
    if (!bot || !target)
        return false;

    float const angle = target->GetAbsoluteAngle(bot);
    float const x = bot->GetPositionX() + std::cos(angle) * RANGED_BACKPEDAL_STEP;
    float const y = bot->GetPositionY() + std::sin(angle) * RANGED_BACKPEDAL_STEP;
    float z = bot->GetPositionZ();

    if (!IsBotMapPosQueryable(bot, x, y))
        return false;

    bot->UpdateAllowedPositionZ(x, y, z);
    return TryMoveToValidatedPoint(bot, x, y, z);
}
}

bool BotPrefersRangedCombat(Player const* bot)
{
    if (!bot)
        return false;

    int32 const prefer = Playerbots::GetCombatPreferRanged();
    if (prefer == 0)
        return false;
    if (prefer > 0)
        return true;

    // Heuristic: ChrSpecialization Caster or Ranged flags → hold gap; else melee.
    if (ChrSpecializationEntry const* spec = bot->GetPrimarySpecializationEntry())
    {
        EnumFlag<ChrSpecializationFlag> const flags = spec->GetFlags();
        if (flags.HasFlag(ChrSpecializationFlag::Ranged) || flags.HasFlag(ChrSpecializationFlag::Caster))
            return true;
        if (flags.HasFlag(ChrSpecializationFlag::Melee))
            return false;
    }

    return false;
}

bool ApplyCombatMovement(Player* bot, Unit* target)
{
    if (!bot || !target || !bot->IsInWorld() || !target->IsInWorld())
        return false;

    if (!BotPrefersRangedCombat(bot))
    {
        // Melee: chase when not already in melee and the approach is walkable.
        if (bot->IsWithinMeleeRange(target))
            return true;

        if (bot->GetMotionMaster()->GetCurrentMovementGeneratorType() == CHASE_MOTION_TYPE)
            return true;

        if (IsApproachPathWalkable(bot, target->GetPositionX(), target->GetPositionY(), target->GetPositionZ()))
        {
            bot->GetMotionMaster()->MoveChase(target);
            return true;
        }

        return false;
    }

    float const prefer = Playerbots::GetCombatRangedDistance();
    float const dist = bot->GetExactDist(target);
    float const inner = prefer * RANGED_BAND_INNER_FACTOR;
    float const outer = prefer * RANGED_BAND_OUTER_FACTOR;

    if (dist < inner)
        return TryBackpedalFromTarget(bot, target);

    if (dist > outer)
    {
        // Approach into the hold band — MoveChase would collapse to melee, so walk toward a
        // point on the preferred ring instead.
        float const angle = bot->GetAbsoluteAngle(target);
        float const x = target->GetPositionX() - std::cos(angle) * prefer;
        float const y = target->GetPositionY() - std::sin(angle) * prefer;
        float z = target->GetPositionZ();

        if (!IsBotMapPosQueryable(bot, x, y))
            return false;

        bot->UpdateAllowedPositionZ(x, y, z);
        return TryMoveToValidatedPoint(bot, x, y, z);
    }

    // Already in the hold band — stop chasing so we don't collapse into melee.
    if (bot->GetMotionMaster()->GetCurrentMovementGeneratorType() == CHASE_MOTION_TYPE)
        bot->StopMoving();

    return true;
}
