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
#include "Player.h"
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

    // In combat: only step in when the bot has no victim (it got aggroed, or its target died
    // while other attackers remain) — otherwise let the current auto-attack run.
    if (bot->IsInCombat())
        return bot->GetVictim() == nullptr;

    return FindNearbyAttackableUnit(bot, ATTACK_SEARCH_RADIUS) != nullptr;
}

bool AttackAnythingAction::Execute(Event /*event*/)
{
    Player* bot = GetBot();
    if (!bot)
        return false;

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

    if (!target)
        target = FindNearbyAttackableUnit(bot, ATTACK_SEARCH_RADIUS);

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
