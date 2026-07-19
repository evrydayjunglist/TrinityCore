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

#include "AttackAction.h"
#include "AttackValidity.h"
#include "BotPlayerbotAI.h"
#include "CombatPositioning.h"
#include "CombatTargetSelect.h"
#include "Player.h"
#include "Playerbots.h"
#include "PlayerbotsConfig.h"
#include "Unit.h"

bool HasAttackableMasterTarget(BotPlayerbotAI* botAI, Player* bot)
{
    // Gate 12: keep the helper name for callers, but accept master target OR attackers.
    return HasValidCombatTarget(botAI, bot);
}

bool AttackMyTargetAction::IsUseful()
{
    Player* bot = GetBot();
    if (!bot || !bot->IsAlive() || !bot->IsInWorld())
        return false;

    // While actively fleeing, yield the tick to FleeAction (higher relevance) — don't re-chase.
    if (_botAI && _botAI->GetAiObjectContext() && AI_VALUE(bool, "is fleeing") &&
        AI_VALUE(uint8, "health") <= Playerbots::GetCombatFleeHealthExitPct())
        return false;

    return HasValidCombatTarget(_botAI, bot);
}

bool AttackMyTargetAction::Execute(Event /*event*/)
{
    Player* bot = GetBot();
    if (!bot)
        return false;

    Unit* target = SelectCombatTarget(_botAI, bot);
    if (!IsValidAttackTarget(bot, target))
        return false;

    // Leaving flee state once we re-engage a valid target above the exit band (or attackers gone
    // already cleared it in FleeHealthTrigger).
    if (_botAI && _botAI->GetAiObjectContext() && AI_VALUE(bool, "is fleeing"))
        SET_AI_VALUE(bool, "is fleeing", false);

    bot->SetSelection(target->GetGUID());

    if (!bot->HasInArc(float(M_PI), target))
        bot->SetFacingToObject(target);

    if (!bot->Attack(target, true))
        return false;

    ApplyCombatMovement(bot, target);
    return true;
}
