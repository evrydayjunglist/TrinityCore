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

#include "StarterValues.h"
#include "AttackValidity.h"
#include "BotPlayerbotAI.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "Playerbots.h"
#include "Unit.h"

#include <cstdlib>

Unit* CurrentTargetValue::Calculate()
{
    Player* bot = GetBot();
    if (!bot || !bot->IsInWorld())
        return nullptr;

    if (Unit* victim = bot->GetVictim())
    {
        // Accepted caveat (handoff playerbots-duel-end-friendly-attack): CalculatedValue
        // side effect — AttackStop when GetVictim fails Gate 12 IsValidAttackTarget so
        // stale post-duel melee clears even when cast Execute never runs. Prefer a
        // leave-combat action later only if PvE LOS chase flicker shows up in playtest.
        if (!IsValidAttackTarget(bot, victim))
        {
            bot->AttackStop();
            victim = nullptr;
        }
        else
            return victim;
    }

    ObjectGuid const targetGuid = bot->GetTarget();
    if (!targetGuid)
        return nullptr;

    return ObjectAccessor::GetUnit(*bot, targetGuid);
}

Unit* MasterTargetValue::Calculate()
{
    Player* bot = GetBot();
    if (!bot || !_botAI)
        return nullptr;

    Player* master = _botAI->GetMaster();
    if (!master || !master->IsInWorld())
        return nullptr;

    ObjectGuid const targetGuid = master->GetTarget();
    if (!targetGuid)
        return nullptr;

    return ObjectAccessor::GetUnit(*bot, targetGuid);
}

GuidVector AttackersValue::Calculate()
{
    GuidVector result;
    Player* bot = GetBot();
    if (!bot || !bot->IsInWorld())
        return result;

    // Minimal TC scan: Unit::getAttackers() — no threat-table invention (Gate 12 can mature).
    for (Unit* attacker : bot->getAttackers())
    {
        if (!attacker || !attacker->IsInWorld() || !attacker->IsAlive())
            continue;
        result.push_back(attacker->GetGUID());
    }
    return result;
}

uint32 AttackerCountValue::Calculate()
{
    return static_cast<uint32>(AI_VALUE(GuidVector, "attackers").size());
}

uint8 HealthValue::Calculate()
{
    Player* bot = GetBot();
    if (!bot)
        return 0;
    return static_cast<uint8>(bot->GetHealthPct());
}

float DistanceValue::Calculate()
{
    Player* bot = GetBot();
    if (!bot || !bot->IsInWorld())
        return 0.0f;

    std::string const& qualifier = GetQualifier();
    Unit* target = nullptr;

    if (qualifier == "master")
    {
        if (!_botAI)
            return 0.0f;
        target = _botAI->GetMaster();
    }
    else
    {
        // Default / "current target" — same semantics Follow/Attack prove-on needs.
        target = AI_VALUE(Unit*, "current target");
    }

    if (!target || !target->IsInWorld())
        return 0.0f;

    // Preserve Gate 8 FollowAction GetExactDist (3D) semantics — not 2D.
    return bot->GetExactDist(target);
}

bool HasAuraValue::Calculate()
{
    Player* bot = GetBot();
    if (!bot)
        return false;

    uint32 const spellId = static_cast<uint32>(std::strtoul(GetQualifier().c_str(), nullptr, 10));
    if (!spellId)
        return false;

    return bot->HasAura(spellId);
}

bool TargetHasAuraValue::Calculate()
{
    Unit* target = AI_VALUE(Unit*, "current target");
    if (!target)
        return false;

    uint32 const spellId = static_cast<uint32>(std::strtoul(GetQualifier().c_str(), nullptr, 10));
    if (!spellId)
        return false;

    return target->HasAura(spellId);
}

bool InMeleeRangeValue::Calculate()
{
    Player* bot = GetBot();
    if (!bot)
        return false;

    Unit* target = AI_VALUE(Unit*, "current target");
    if (!target)
        return false;

    return bot->IsWithinMeleeRange(target);
}

bool IsCastingValue::Calculate()
{
    Player* bot = GetBot();
    if (!bot)
        return false;

    // Match FollowAction Gate 8 guard: IsNonMeleeSpellCast(false).
    return bot->IsNonMeleeSpellCast(false);
}
