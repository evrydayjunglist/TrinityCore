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
#include "ObjectAccessor.h"
#include "Player.h"
#include "Unit.h"

namespace
{
Unit* GetMasterAttackTarget(BotPlayerbotAI* botAI, Player* bot)
{
    if (!botAI || !bot)
        return nullptr;

    Player* master = botAI->GetMaster();
    if (!master || !master->IsInWorld())
        return nullptr;

    ObjectGuid const targetGuid = master->GetTarget();
    if (!targetGuid)
        return nullptr;

    return ObjectAccessor::GetUnit(*bot, targetGuid);
}
}

bool HasAttackableMasterTarget(BotPlayerbotAI* botAI, Player* bot)
{
    Unit* target = GetMasterAttackTarget(botAI, bot);
    return IsValidAttackTarget(bot, target);
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

    Unit* target = GetMasterAttackTarget(_botAI, bot);
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
    // without this a master-alt bot only lands hits when the mob walks into it.
    bot->GetMotionMaster()->MoveChase(target);
    return true;
}
