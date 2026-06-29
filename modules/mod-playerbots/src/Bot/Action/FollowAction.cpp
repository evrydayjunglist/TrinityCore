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

#include "FollowAction.h"
#include "AttackAction.h"
#include "BotPlayerbotAI.h"
#include "MotionMaster.h"
#include "Player.h"
#include "PlayerbotsConfig.h"
#include "Unit.h"

bool FollowAction::IsUseful()
{
    Player* bot = GetBot();
    if (!bot || !_botAI)
        return false;

    if (HasAttackableMasterTarget(_botAI, bot))
        return false;

    Player* master = _botAI->GetMaster();
    if (!master || !master->IsInWorld() || !bot->IsInWorld())
        return false;

    if (bot->GetMapId() != master->GetMapId())
        return false;

    if (master->HasUnitState(UNIT_STATE_IN_FLIGHT))
        return false;

    if (bot->GetCurrentSpell(CURRENT_CHANNELED_SPELL))
        return false;

    float const followDistance = Playerbots::GetFollowDistance();
    return bot->GetExactDist(master) > followDistance;
}

bool FollowAction::Execute(Event /*event*/)
{
    Player* bot = GetBot();
    if (!bot || !_botAI)
        return false;

    Player* master = _botAI->GetMaster();
    if (!master || !master->IsInWorld() || bot->GetMapId() != master->GetMapId())
        return false;

    float const followDistance = Playerbots::GetFollowDistance();
    bot->GetMotionMaster()->MoveFollow(master, followDistance);
    return true;
}
