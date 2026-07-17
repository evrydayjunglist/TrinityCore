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

#include "GroupActions.h"
#include "BotPlayerbotAI.h"
#include "Group.h"
#include "Log.h"
#include "Opcodes.h"
#include "PartyPackets.h"
#include "Player.h"
#include "PlayerbotsConfig.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include <utility>

namespace
{
bool HasMasterInvite(BotPlayerbotAI* botAI, Player*& bot, Player*& master)
{
    if (!botAI)
        return false;

    bot = botAI->GetBot();
    master = botAI->GetMaster();
    if (!bot || !master)
        return false;

    Group* invite = bot->GetGroupInvite();
    return invite && invite->GetLeaderGUID() == master->GetGUID();
}
}

bool AcceptInvitationAction::IsUseful()
{
    Player* bot = nullptr;
    Player* master = nullptr;
    return HasMasterInvite(_botAI, bot, master);
}

bool AcceptInvitationAction::Execute(Event /*event*/)
{
    Player* bot = nullptr;
    Player* master = nullptr;
    if (!HasMasterInvite(_botAI, bot, master))
        return false;

    // Accept server-side through TC's own opcode handler — the same path the client's Accept
    // button drives. For a not-yet-created group, HandlePartyInviteResponseOpcode calls
    // Group::Create + GroupMgr::AddGroup internally. No outbound packet is needed on the
    // socketless bot side; the fields are read straight off the packet object.
    WorldPacket packet(CMSG_PARTY_INVITE_RESPONSE);
    WorldPackets::Party::PartyInviteResponse response(std::move(packet));
    response.Accept = true;
    bot->GetSession()->HandlePartyInviteResponseOpcode(response);

    Group* group = bot->GetGroup();
    bool const joined = group && group->IsMember(master->GetGUID());

    if (Playerbots::GetLogLevel() >= 1)
        TC_LOG_DEBUG("playerbots", "AcceptInvitationAction bot={} master={} joined={}",
            bot->GetName(), master->GetName(), joined ? "yes" : "no");

    // The bot already runs the "follow" strategy while it has a master (BotPlayerbotAI::
    // ResetStrategies), so in-group following is already active — no strategy change needed here.
    return joined;
}

bool ResetAiAction::IsUseful()
{
    return _botAI && _botAI->GetBot();
}

bool ResetAiAction::Execute(Event /*event*/)
{
    if (!_botAI)
        return false;

    // V1: AC WorldPacketHandlerStrategy "group set leader" → "reset botAI" intent only.
    // Full AC ResetAiAction (FindNewMaster / repository wipe / TellMaster) stays out of scope.
    _botAI->ResetStrategies();

    if (Playerbots::GetLogLevel() >= 1)
    {
        Player* bot = _botAI->GetBot();
        TC_LOG_DEBUG("playerbots", "ResetAiAction bot={} strategies reset",
            bot ? bot->GetName() : "?");
    }

    return true;
}
