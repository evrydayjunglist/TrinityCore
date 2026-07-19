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

#include "CombatTargetSelect.h"
#include "AiObjectContext.h"
#include "AttackValidity.h"
#include "Bot/Engine/Value.h"
#include "BotPlayerbotAI.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "Unit.h"

Unit* SelectNearestValidAttacker(BotPlayerbotAI* botAI, Player* bot)
{
    if (!botAI || !bot || !bot->IsInWorld())
        return nullptr;

    AiObjectContext* context = botAI->GetAiObjectContext();
    if (!context)
        return nullptr;

    Value<GuidVector>* attackersValue = context->GetValue<GuidVector>("attackers");
    if (!attackersValue)
        return nullptr;

    GuidVector const attackers = attackersValue->Get();
    Unit* best = nullptr;
    float bestDist = 0.0f;

    for (ObjectGuid const& guid : attackers)
    {
        Unit* attacker = ObjectAccessor::GetUnit(*bot, guid);
        if (!IsValidAttackTarget(bot, attacker))
            continue;

        float const dist = bot->GetExactDist(attacker);
        if (!best || dist < bestDist)
        {
            best = attacker;
            bestDist = dist;
        }
    }

    return best;
}

Unit* SelectCombatTarget(BotPlayerbotAI* botAI, Player* bot)
{
    if (!botAI || !bot)
        return nullptr;

    AiObjectContext* context = botAI->GetAiObjectContext();
    if (!context)
        return nullptr;

    // Priority 1 — master's selection (Gate 8 assist feel).
    if (Value<Unit*>* masterTargetValue = context->GetValue<Unit*>("master target"))
        if (Unit* masterTarget = masterTargetValue->Get())
            if (IsValidAttackTarget(bot, masterTarget))
                return masterTarget;

    // Priority 2 — hit back when punched (even if master selection is empty/stale).
    return SelectNearestValidAttacker(botAI, bot);
}

bool HasValidCombatTarget(BotPlayerbotAI* botAI, Player* bot)
{
    return SelectCombatTarget(botAI, bot) != nullptr;
}
