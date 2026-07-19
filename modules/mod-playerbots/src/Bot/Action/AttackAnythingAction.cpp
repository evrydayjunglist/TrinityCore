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

#include "AttackAnythingAction.h"
#include "AttackValidity.h"
#include "BotPlayerbotAI.h"
#include "CombatPositioning.h"
#include "CombatTargetSelect.h"
#include "Player.h"
#include "Playerbots.h"
#include "PlayerbotsConfig.h"
#include "Unit.h"

namespace
{
// Same live-search radius Gate 10's grind behavior used ("what to attack right now").
constexpr float ATTACK_SEARCH_RADIUS = 30.0f;
}

bool AttackAnythingAction::IsUseful()
{
    Player* bot = GetBot();
    if (!bot || !bot->IsInWorld() || !bot->IsAlive())
        return false;

    // Yield to Gate 12 flee while low-HP hysteresis is active.
    if (_botAI && _botAI->GetAiObjectContext() && AI_VALUE(bool, "is fleeing") &&
        AI_VALUE(uint8, "health") <= Playerbots::GetCombatFleeHealthExitPct())
        return false;

    // In combat: step in when the bot has no victim (it got aggroed, or its target died while other
    // attackers remain), OR while it still holds a valid victim it hasn't finished — so the combat
    // chase keeps *owning movement* for the whole fight instead of an RPG-travel MovePoint clobbering
    // it and parking the bot just out of melee range. A melee mob closes to the bot on its own so this
    // never mattered for them; a *ranged* attacker (Northwatch Scout 39317, quest 25172) holds its
    // distance and shoots, so a bot that cedes movement mid-fight stands at ~6yd, never swings, and
    // dies. Confirmed via C0 instrumentation (moveGen=POINT while a scout victim sat 5-6yd out). See
    // playerbots-rpg-combat-ranged-attacker-engagement-handoff.md.
    if (bot->IsInCombat())
    {
        Unit* victim = bot->GetVictim();
        return !victim || IsValidAttackTarget(bot, victim);
    }

    // Out of combat: engage a nearby hostile, OR a nearby *neutral* creature that a live quest
    // kill objective wants dead (the narrow widening — see the quest-kill searcher). Neutral
    // inclusion is gated entirely on the objective, so a bot with no matching kill quest still
    // behaves exactly as before (hostile-only) and never griefs neutral wildlife.
    return FindNearbyAttackableUnit(bot, ATTACK_SEARCH_RADIUS) != nullptr ||
        FindNearbyQuestKillTarget(bot, Playerbots::GetRpgQuestKillSearchRadius()) != nullptr;
}

bool AttackAnythingAction::Execute(Event /*event*/)
{
    Player* bot = GetBot();
    if (!bot)
        return false;

    // Already fighting a valid target: keep it (don't thrash) and re-assert Gate 12 positioning so
    // RPG travel can't steal movement mid-fight. See
    // playerbots-rpg-combat-ranged-attacker-engagement-handoff.md.
    if (Unit* victim = bot->GetVictim())
    {
        if (bot->IsInCombat() && IsValidAttackTarget(bot, victim))
        {
            if (_botAI && _botAI->GetAiObjectContext() && AI_VALUE(bool, "is fleeing"))
                SET_AI_VALUE(bool, "is fleeing", false);

            if (!bot->HasInArc(float(M_PI), victim))
                bot->SetFacingToObject(victim);

            ApplyCombatMovement(bot, victim);
            return true;
        }
    }

    Unit* target = nullptr;

    // Fight back first via Gate 12 AI_VALUE("attackers") helper (shared with master-alt path).
    if (bot->IsInCombat() && !bot->GetVictim())
        target = SelectNearestValidAttacker(_botAI, bot);

    // Prefer a nearby hostile; fall back to a neutral creature a quest kill objective wants dead.
    if (!target)
        target = FindNearbyAttackableUnit(bot, ATTACK_SEARCH_RADIUS);

    if (!target)
        target = FindNearbyQuestKillTarget(bot, Playerbots::GetRpgQuestKillSearchRadius());

    if (!IsValidAttackTarget(bot, target))
        return false;

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
