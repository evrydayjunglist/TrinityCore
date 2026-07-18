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

#include "Bot/PacketHandler/BotPacketSignal.h"
#include "Bot/Packet/BotPacketParse.h"
#include "Log.h"
#include "Opcodes.h"
#include "PlayerbotsConfig.h"
#include "Bot/Packet/BotLFGRoleCheckUpdatePacket.h"
#include "Bot/Packet/BotLFGProposalUpdatePacket.h"
#include "LFG.h"
#include "LFGMgr.h"
#include "Player.h"

namespace Playerbots::PacketHandler
{
void HandleLfgRoleCheckUpdate(BotPlayerbotAI& ai, BotPlayerbotAI::QueuedSignal& signal)
{

        // Cleared unless Layer 2 OK (sLFGMgr ROLECHECK). No stash for role check.
        signal.Name.clear();

        Playerbots::PacketParse::LFGRoleCheckUpdatePayload parsed;
        if (!Playerbots::PacketParse::TryReadLFGRoleCheckUpdate(*signal.Packet, parsed))
            return;

        Player* bot = ai.GetBot();
        ObjectGuid const botGuid = bot ? bot->GetGUID() : ObjectGuid::Empty;
        lfg::LfgState const liveState = bot ? sLFGMgr->GetState(botGuid) : lfg::LFG_STATE_NONE;
        if (liveState != lfg::LFG_STATE_ROLECHECK)
        {
            TC_LOG_ERROR("playerbots.packet",
                "BotPacketParse SMSG_LFG_ROLE_CHECK_UPDATE Layer-2 state mismatch bot={} liveState={} verifiedBuild={}",
                bot ? bot->GetName() : "?",
                uint32(liveState),
                Playerbots::PacketParse::VERIFIED_BUILD);
            return;
        }

        if (parsed.RoleCheckStatus > uint8(lfg::LFG_ROLECHECK_NO_ROLE))
        {
            TC_LOG_ERROR("playerbots.packet",
                "BotPacketParse SMSG_LFG_ROLE_CHECK_UPDATE Layer-2 bad RoleCheckStatus bot={} status={} verifiedBuild={}",
                bot->GetName(),
                uint32(parsed.RoleCheckStatus),
                Playerbots::PacketParse::VERIFIED_BUILD);
            return;
        }

        // Soft: prefer bot in a group (HandleLfgSetRolesOpcode requires it).
        if (!bot->GetGroup() && Playerbots::GetLogLevel() >= 1)
            TC_LOG_DEBUG("playerbots.packet",
                "BotPacketParse SMSG_LFG_ROLE_CHECK_UPDATE soft no group bot={}",
                bot->GetName());

        // Soft: Members non-empty and includes bot Guid.
        bool memberIncludesBot = false;
        for (auto const& member : parsed.Members)
        {
            if (member.Guid == botGuid)
            {
                memberIncludesBot = true;
                break;
            }
        }
        if ((parsed.Members.empty() || !memberIncludesBot) && Playerbots::GetLogLevel() >= 1)
            TC_LOG_DEBUG("playerbots.packet",
                "BotPacketParse SMSG_LFG_ROLE_CHECK_UPDATE soft Members bot={} count={} includesBot={}",
                bot->GetName(),
                parsed.Members.size(),
                memberIncludesBot ? "yes" : "no");

        if (Playerbots::GetLogLevel() >= 1)
            TC_LOG_DEBUG("playerbots.packet",
                "BotPacketParse SMSG_LFG_ROLE_CHECK_UPDATE ok bot={} status={} members={} joinSlots={} beginning={}",
                bot->GetName(),
                uint32(parsed.RoleCheckStatus),
                parsed.Members.size(),
                parsed.JoinSlots.size(),
                parsed.IsBeginning ? "yes" : "no");

        signal.Name = "lfg role check";
}

void HandleLfgProposalUpdate(BotPlayerbotAI& ai, BotPlayerbotAI::QueuedSignal& signal)
{

        // Cleared unless Layer 2 OK (sLFGMgr PROPOSAL + ProposalID). Stash for accept.
        signal.Name.clear();
        ai.ClearPendingLfgProposal();

        Playerbots::PacketParse::LFGProposalUpdatePayload parsed;
        if (!Playerbots::PacketParse::TryReadLFGProposalUpdate(*signal.Packet, parsed))
            return;

        Player* bot = ai.GetBot();
        ObjectGuid const botGuid = bot ? bot->GetGUID() : ObjectGuid::Empty;
        lfg::LfgState const liveState = bot ? sLFGMgr->GetState(botGuid) : lfg::LFG_STATE_NONE;
        if (liveState != lfg::LFG_STATE_PROPOSAL)
        {
            TC_LOG_ERROR("playerbots.packet",
                "BotPacketParse SMSG_LFG_PROPOSAL_UPDATE Layer-2 state mismatch bot={} liveState={} verifiedBuild={}",
                bot ? bot->GetName() : "?",
                uint32(liveState),
                Playerbots::PacketParse::VERIFIED_BUILD);
            return;
        }

        if (!parsed.ProposalID)
        {
            TC_LOG_ERROR("playerbots.packet",
                "BotPacketParse SMSG_LFG_PROPOSAL_UPDATE Layer-2 ProposalID=0 bot={} verifiedBuild={}",
                bot->GetName(),
                Playerbots::PacketParse::VERIFIED_BUILD);
            return;
        }

        // Soft: Players contains Me == true.
        bool hasMe = false;
        for (auto const& player : parsed.Players)
        {
            if (player.Me)
            {
                hasMe = true;
                break;
            }
        }
        if (!hasMe && Playerbots::GetLogLevel() >= 1)
            TC_LOG_DEBUG("playerbots.packet",
                "BotPacketParse SMSG_LFG_PROPOSAL_UPDATE soft no Me entry bot={} players={}",
                bot->GetName(),
                parsed.Players.size());

        // Soft: Ticket.RequesterGuid == bot when ticket present.
        if (!parsed.Ticket.RequesterGuid.IsEmpty() && parsed.Ticket.RequesterGuid != botGuid &&
            Playerbots::GetLogLevel() >= 1)
        {
            TC_LOG_DEBUG("playerbots.packet",
                "BotPacketParse SMSG_LFG_PROPOSAL_UPDATE soft Ticket.RequesterGuid mismatch bot={} ticket={}",
                bot->GetName(),
                parsed.Ticket.RequesterGuid.ToString());
        }

        if (Playerbots::GetLogLevel() >= 1)
            TC_LOG_DEBUG("playerbots.packet",
                "BotPacketParse SMSG_LFG_PROPOSAL_UPDATE ok bot={} proposalId={} slot={} players={} ticket={}",
                bot->GetName(),
                parsed.ProposalID,
                parsed.Slot,
                parsed.Players.size(),
                parsed.Ticket.RequesterGuid.ToString());

        BotPlayerbotAI::PendingLfgProposal pending;
        pending.Ticket = parsed.Ticket;
        pending.InstanceID = parsed.InstanceID;
        pending.ProposalID = parsed.ProposalID;
        ai.SetPendingLfgProposal(std::move(pending));
        signal.Name = "lfg proposal";
}

} // namespace Playerbots::PacketHandler
