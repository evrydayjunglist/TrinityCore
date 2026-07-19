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
#include "MotionMaster.h"
#include "Player.h"
#include "Playerbots.h"
#include "SafeMovement.h"
#include "Unit.h"

bool HasAttackableMasterTarget(BotPlayerbotAI* botAI, Player* bot)
{
    if (!botAI || !bot)
        return false;

    AiObjectContext* context = botAI->GetAiObjectContext();
    if (!context)
        return false;

    Value<Unit*>* masterTarget = context->GetValue<Unit*>("master target");
    if (!masterTarget)
        return false;

    return IsValidAttackTarget(bot, masterTarget->Get());
}

bool AttackMyTargetAction::IsUseful()
{
    return HasAttackableMasterTarget(_botAI, GetBot());
}

bool AttackMyTargetAction::Execute(Event /*event*/)
{
    Player* bot = GetBot();
    if (!bot)
        return false;

    Unit* target = AI_VALUE(Unit*, "master target");
    if (!IsValidAttackTarget(bot, target))
        return false;

    bot->SetSelection(target->GetGUID());

    if (!bot->HasInArc(float(M_PI), target))
        bot->SetFacingToObject(target);

    if (!bot->Attack(target, true))
        return false;

    // Close to melee range and stay on the target (core ChaseMovementGenerator). MoveChase
    // installs an active-slot generator that supersedes the +follow FollowMovementGenerator, so
    // no explicit follow Clear() is needed. Mirrors AttackAnythingAction (random/newrpg bots) —
    // without this a master-alt bot only lands hits when the mob walks into it. Same slope gate
    // as AttackAnythingAction — this action's IsUseful() stays true every tick the master keeps
    // its target, so an unwalkable approach gets re-checked (and can clear) as the fight moves,
    // unlike the once-per-engagement AttackAnythingAction check.
    // Gate 11: "in melee range" reads current target after SetSelection/Attack (same unit).
    if (AI_VALUE(bool, "in melee range") ||
        IsApproachPathWalkable(bot, target->GetPositionX(), target->GetPositionY(), target->GetPositionZ()))
        bot->GetMotionMaster()->MoveChase(target);

    return true;
}
