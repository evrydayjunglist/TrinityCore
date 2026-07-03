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

#include "GroupTriggers.h"
#include "BotPlayerbotAI.h"
#include "Group.h"
#include "Player.h"

bool GroupInviteTrigger::IsActive()
{
    if (!_botAI)
        return false;

    Player* bot = _botAI->GetBot();
    Player* master = _botAI->GetMaster();
    if (!bot || !master)
        return false;

    Group* invite = bot->GetGroupInvite();
    if (!invite)
        return false;

    // Only auto-accept an invite that came from this bot's own master (retail/AC-like: the
    // master invites through the normal party UI). The leader GUID is set at invite time via
    // Group::AddLeaderInvite even before the group is created. Random bots run the "newrpg"
    // strategy and never reach this trigger — AC gates their any-player accept behind
    // PlayerbotSecurity, which is NYI in this fork.
    return invite->GetLeaderGUID() == master->GetGUID();
}
