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
#include "Bot/Packet/BotGroupDestroyedPacket.h"
#include "Bot/Packet/BotGroupNewLeaderPacket.h"
#include "Bot/Packet/BotMoveSetRunSpeedPacket.h"
#include "Bot/Packet/BotPartyCommandResultPacket.h"
#include "Group.h"
#include "Player.h"
#include "SharedDefines.h"
#include "Unit.h"
#include "WorldSession.h"
#include <cmath>

namespace Playerbots::PacketHandler
{
void HandleGroupNewLeader(BotPlayerbotAI& ai, BotPlayerbotAI::QueuedSignal& signal)
{

        // Cleared unless Layer 2 OK — do not fire "reset botAI" on mis-parse / mismatch.
        signal.Name.clear();

        Playerbots::PacketParse::GroupNewLeaderPayload parsed;
        if (!Playerbots::PacketParse::TryReadGroupNewLeader(*signal.Packet, parsed))
            return;

        Player* bot = ai.GetBot();
        Group* group = bot ? bot->GetGroup() : nullptr;
        char const* liveLeaderName = group ? group->GetLeaderName() : nullptr;
        bool const nameOk = group && liveLeaderName && !parsed.Name.empty() &&
            parsed.Name == liveLeaderName;
        bool const categoryOk = group &&
            parsed.PartyIndex == int8(group->GetGroupCategory());
        if (!nameOk || !categoryOk)
        {
            TC_LOG_ERROR("playerbots.packet",
                "BotPacketParse SMSG_GROUP_NEW_LEADER Layer-2 mismatch bot={} parsedName='{}' parsedPartyIndex={} liveLeader='{}' liveCategory={} verifiedBuild={}",
                bot ? bot->GetName() : "?",
                parsed.Name,
                int32(parsed.PartyIndex),
                liveLeaderName ? liveLeaderName : "none",
                group ? int32(group->GetGroupCategory()) : -1,
                Playerbots::PacketParse::VERIFIED_BUILD);
            return;
        }

        signal.Name = "group set leader";

        if (Playerbots::GetLogLevel() >= 1)
            TC_LOG_DEBUG("playerbots.packet",
                "BotPacketParse SMSG_GROUP_NEW_LEADER ok bot={} leader='{}' partyIndex={}",
                bot->GetName(),
                parsed.Name,
                int32(parsed.PartyIndex));
}

void HandleGroupDestroyed(BotPlayerbotAI& ai, BotPlayerbotAI::QueuedSignal& signal)
{
        // Cleared unless Layer 2 OK — do not fire "reset botAI" on mis-parse / stale group.
        signal.Name.clear();

        if (!Playerbots::PacketParse::TryReadGroupDestroyed(*signal.Packet))
            return;

        Player* bot = ai.GetBot();
        if (!bot)
            return;

        // Group::Disband clears SetGroup(nullptr) before SendDirectMessage(GroupDestroyed)
        // (unless hideDestroy). Prefer dual = no current group.
        if (bot->GetGroup())
        {
            TC_LOG_ERROR("playerbots.packet",
                "BotPacketParse SMSG_GROUP_DESTROYED Layer-2 still grouped bot={} verifiedBuild={}",
                bot->GetName(),
                Playerbots::PacketParse::VERIFIED_BUILD);
            return;
        }

        // Soft: original-group / loading oddities (e.g. BG/BF raid path) — DEBUG only.
        if (bot->GetOriginalGroup() && Playerbots::GetLogLevel() >= 1)
            TC_LOG_DEBUG("playerbots.packet",
                "BotPacketParse SMSG_GROUP_DESTROYED soft original-group still set bot={}",
                bot->GetName());

        if (bot->IsLoading() && Playerbots::GetLogLevel() >= 1)
            TC_LOG_DEBUG("playerbots.packet",
                "BotPacketParse SMSG_GROUP_DESTROYED soft bot loading bot={}",
                bot->GetName());

        signal.Name = "group destroyed";

        if (Playerbots::GetLogLevel() >= 1)
            TC_LOG_DEBUG("playerbots.packet",
                "BotPacketParse SMSG_GROUP_DESTROYED ok bot={}",
                bot->GetName());
}

void HandleMoveSetRunSpeed(BotPlayerbotAI& ai, BotPlayerbotAI::QueuedSignal& signal)
{

        // Cleared unless Layer 2 OK — do not fire "check mount state" on mis-parse / mismatch.
        signal.Name.clear();

        Playerbots::PacketParse::MoveSetRunSpeedPayload parsed;
        if (!Playerbots::PacketParse::TryReadMoveSetRunSpeed(*signal.Packet, parsed))
            return;

        Player* bot = ai.GetBot();
        if (!bot || parsed.MoverGUID.IsEmpty())
        {
            TC_LOG_ERROR("playerbots.packet",
                "BotPacketParse SMSG_MOVE_SET_RUN_SPEED Layer-2 empty mover bot={} verifiedBuild={}",
                bot ? bot->GetName() : "?",
                Playerbots::PacketParse::VERIFIED_BUILD);
            return;
        }

        // Self-session force-set only; ignore (no reaction) if somehow not the bot mover.
        if (parsed.MoverGUID != bot->GetGUID())
        {
            if (Playerbots::GetLogLevel() >= 1)
                TC_LOG_DEBUG("playerbots.packet",
                    "BotPacketParse SMSG_MOVE_SET_RUN_SPEED ignored (not self) bot={} mover={}",
                    bot->GetName(),
                    parsed.MoverGUID.ToString());
            return;
        }

        constexpr float kRunSpeedEpsilon = 0.01f;
        float const liveSpeed = bot->GetSpeed(MOVE_RUN);
        if (std::fabs(parsed.Speed - liveSpeed) > kRunSpeedEpsilon)
        {
            TC_LOG_ERROR("playerbots.packet",
                "BotPacketParse SMSG_MOVE_SET_RUN_SPEED Layer-2 mismatch bot={} parsedSpeed={} liveSpeed={} verifiedBuild={}",
                bot->GetName(),
                parsed.Speed,
                liveSpeed,
                Playerbots::PacketParse::VERIFIED_BUILD);
            return;
        }

        signal.Name = "check mount state";

        if (Playerbots::GetLogLevel() >= 1)
            TC_LOG_DEBUG("playerbots.packet",
                "BotPacketParse SMSG_MOVE_SET_RUN_SPEED ok bot={} speed={} seq={}",
                bot->GetName(),
                parsed.Speed,
                parsed.SequenceIndex);
}

void HandlePartyCommandResult(BotPlayerbotAI& ai, BotPlayerbotAI::QueuedSignal& signal)
{

        // Cleared unless Layer 2 OK. Enable=0 / mismatch: no poll dual.
        signal.Name.clear();
        ai.ClearPendingPartyCommand();

        Playerbots::PacketParse::PartyCommandResultPayload parsed;
        if (!Playerbots::PacketParse::TryReadPartyCommandResult(*signal.Packet, parsed))
            return;

        if (!Playerbots::PacketParse::IsKnownPartyOperation(parsed.Command))
        {
            TC_LOG_ERROR("playerbots.packet",
                "BotPacketParse SMSG_PARTY_COMMAND_RESULT Layer-2 unknown Command bot={} command={} verifiedBuild={}",
                ai.GetBot() ? ai.GetBot()->GetName() : "?",
                uint32(parsed.Command),
                Playerbots::PacketParse::VERIFIED_BUILD);
            return;
        }

        // Documented PartyResult max is ERR_PARTY_LFG_TELEPORT_IN_COMBAT (30).
        // Outside that → ERROR. Gaps inside the range (10/11) → soft parse-ok, no reaction.
        if (parsed.Result > uint8(ERR_PARTY_LFG_TELEPORT_IN_COMBAT))
        {
            TC_LOG_ERROR("playerbots.packet",
                "BotPacketParse SMSG_PARTY_COMMAND_RESULT Layer-2 Result out of documented range bot={} result={} verifiedBuild={}",
                ai.GetBot() ? ai.GetBot()->GetName() : "?",
                uint32(parsed.Result),
                Playerbots::PacketParse::VERIFIED_BUILD);
            return;
        }

        if (!Playerbots::PacketParse::IsKnownPartyResult(parsed.Result))
        {
            if (Playerbots::GetLogLevel() >= 1)
                TC_LOG_DEBUG("playerbots.packet",
                    "BotPacketParse SMSG_PARTY_COMMAND_RESULT ok (unknown-but-plausible Result, no reaction) bot={} command={} result={} name='{}'",
                    ai.GetBot() ? ai.GetBot()->GetName() : "?",
                    uint32(parsed.Command),
                    uint32(parsed.Result),
                    parsed.Name);
            return;
        }

        Player* bot = ai.GetBot();
        Player* master = ai.GetMaster();
        // Soft leave dual: do not hard-require GetGroup() after LEAVE OK (group may be gone).
        // Prefer Name matches bot or master when present; mismatch is not a Layer-2 ERROR.
        if (parsed.Command == uint8(PARTY_OP_LEAVE) &&
            parsed.Result == uint8(ERR_PARTY_RESULT_OK) &&
            !parsed.Name.empty() &&
            bot &&
            parsed.Name != bot->GetName() &&
            (!master || parsed.Name != master->GetName()) &&
            Playerbots::GetLogLevel() >= 1)
        {
            TC_LOG_DEBUG("playerbots.packet",
                "BotPacketParse SMSG_PARTY_COMMAND_RESULT LEAVE OK soft Name dual bot={} name='{}' master='{}'",
                bot->GetName(),
                parsed.Name,
                master ? master->GetName() : "none");
        }

        BotPlayerbotAI::PendingPartyCommand stash;
        stash.Command = parsed.Command;
        stash.Result = parsed.Result;
        stash.Name = parsed.Name;
        ai.SetPendingPartyCommand(std::move(stash));
        signal.Name = "party command";

        if (Playerbots::GetLogLevel() >= 1)
            TC_LOG_DEBUG("playerbots.packet",
                "BotPacketParse SMSG_PARTY_COMMAND_RESULT ok bot={} command={} result={} name='{}' resultData={} resultGuid={}",
                bot ? bot->GetName() : "?",
                uint32(parsed.Command),
                uint32(parsed.Result),
                parsed.Name,
                parsed.ResultData,
                parsed.ResultGUID.ToString());
}

} // namespace Playerbots::PacketHandler
