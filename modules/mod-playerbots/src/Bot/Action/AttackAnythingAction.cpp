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
#include "MotionMaster.h"
#include "MovementDefines.h"
#include "Player.h"
#include "PlayerbotsConfig.h"
#include "SafeMovement.h"
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

    // Already fighting a valid target: keep it (don't thrash to a different one) and make sure the
    // combat chase — not a stray RPG-travel MovePoint — owns movement, so the bot actually closes to
    // and holds melee. Re-assert MoveChase only when it isn't already the active generator, to avoid
    // resetting the spline every tick (when already chasing we still consume the tick so RPG travel
    // can't take movement back mid-fight). The walkability gate is unchanged — the SafeMovement slope
    // contract stays exactly as-is (playerbots-bot-wander-ground-clip-handoff.md ⭐ standing watch).
    // Returning here yields the fight to core auto-attack once in melee (MoveChase just holds range).
    // When the victim dies or goes invalid, IsUseful releases and RPG travel resumes. See
    // playerbots-rpg-combat-ranged-attacker-engagement-handoff.md.
    if (Unit* victim = bot->GetVictim())
    {
        if (bot->IsInCombat() && IsValidAttackTarget(bot, victim))
        {
            if (!bot->HasInArc(float(M_PI), victim))
                bot->SetFacingToObject(victim);

            if (bot->GetMotionMaster()->GetCurrentMovementGeneratorType() != CHASE_MOTION_TYPE &&
                IsApproachPathWalkable(bot, victim->GetPositionX(), victim->GetPositionY(), victim->GetPositionZ()))
                bot->GetMotionMaster()->MoveChase(victim);

            return true;
        }
    }

    Unit* target = nullptr;

    // Fight back first: prefer whoever is actually attacking the bot.
    if (bot->IsInCombat() && !bot->GetVictim())
    {
        for (Unit* attacker : bot->getAttackers())
        {
            if (IsValidAttackTarget(bot, attacker))
            {
                target = attacker;
                break;
            }
        }
    }

    // Prefer a nearby hostile; fall back to a neutral creature a quest kill objective wants dead.
    if (!target)
        target = FindNearbyAttackableUnit(bot, ATTACK_SEARCH_RADIUS);

    if (!target)
        target = FindNearbyQuestKillTarget(bot, Playerbots::GetRpgQuestKillSearchRadius());

    if (!IsValidAttackTarget(bot, target))
        return false;

    bot->SetSelection(target->GetGUID());
    if (!bot->HasInArc(float(M_PI), target))
        bot->SetFacingToObject(target);

    if (!bot->Attack(target, true))
        return false;

    // Close to melee range and stay on the target (core ChaseMovementGenerator, same
    // target-relative movement family FollowAction already uses via MoveFollow) — without this
    // a bot only ever lands hits when the mob walks into it. ChaseMovementGenerator builds its
    // own PathGenerator with only the engine's default (55-degree) slope filter, so gate it on
    // SafeMovement's stricter check first — already in melee range needs no chase movement at
    // all and is never blocked by terrain. See SafeMovement.h and
    // playerbots-bot-wander-ground-clip-handoff.md §6.
    if (bot->IsWithinMeleeRange(target) ||
        IsApproachPathWalkable(bot, target->GetPositionX(), target->GetPositionY(), target->GetPositionZ()))
        bot->GetMotionMaster()->MoveChase(target);

    return true;
}
