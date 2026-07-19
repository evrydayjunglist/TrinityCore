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

#include "PartyCommandAction.h"
#include "BotPlayerbotAI.h"
#include "Log.h"
#include "Opcodes.h"
#include "PartyPackets.h"
#include "Player.h"
#include "PlayerbotsConfig.h"
#include "SharedDefines.h"
#include "WorldPacket.h"
#include "WorldSession.h"

bool PartyCommandAction::IsUseful()
{
    return _botAI && _botAI->GetBot() && _botAI->GetPendingPartyCommand().has_value();
}

bool PartyCommandAction::Execute(Event /*event*/)
{
    if (!_botAI)
        return false;

    std::optional<BotPlayerbotAI::PendingPartyCommand> const pending = _botAI->GetPendingPartyCommand();
    _botAI->ClearPendingPartyCommand();
    if (!pending)
        return false;

    Player* bot = _botAI->GetBot();
    if (!bot || !bot->GetSession())
        return false;

    // AC PartyCommandAction leave-follow intent only: LEAVE OK + Name == master.
    // Midnight may deliver LEAVE results only to the leaver's session — this branch is soft/rare.
    Player* master = _botAI->GetMaster();
    bool const leaveFollow = pending->Command == uint8(PARTY_OP_LEAVE) &&
        pending->Result == uint8(ERR_PARTY_RESULT_OK) &&
        master && pending->Name == master->GetName();

    if (!leaveFollow)
    {
        if (Playerbots::GetLogLevel() >= 1)
            TC_LOG_DEBUG("playerbots",
                "PartyCommandAction bot={} signal only command={} result={} name='{}'",
                bot->GetName(),
                uint32(pending->Command),
                uint32(pending->Result),
                pending->Name);
        return true;
    }

    WorldPacket packet(CMSG_LEAVE_GROUP);
    WorldPackets::Party::LeaveGroup leave(std::move(packet));
    bot->GetSession()->HandleLeaveGroupOpcode(leave);

    if (Playerbots::GetLogLevel() >= 1)
        TC_LOG_DEBUG("playerbots",
            "PartyCommandAction bot={} leave-follow master='{}' via HandleLeaveGroupOpcode",
            bot->GetName(),
            master->GetName());

    return true;
}
