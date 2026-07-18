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

#include "BotPartyCommandResultPacket.h"
#include "BotPacketParse.h"
#include "PacketOperators.h"
#include "SharedDefines.h"
#include "WorldSession.h"

namespace Playerbots::PacketParse
{
namespace
{
void ReadPartyCommandResultBody(ByteBuffer& data, PartyCommandResultPayload& out)
{
    // Mirror WorldPackets::Party::PartyCommandResult::Write() field order exactly.
    data >> WorldPackets::SizedString::BitsSize<9>(out.Name);
    data >> WorldPackets::Bits<4>(out.Command);
    data >> WorldPackets::Bits<6>(out.Result);
    data.ResetBitPos();

    data >> out.ResultData;
    data >> out.ResultGUID;
    data >> WorldPackets::SizedString::Data(out.Name);
}
}

bool IsKnownPartyOperation(uint8 command)
{
    switch (PartyOperation(command))
    {
        case PARTY_OP_INVITE:
        case PARTY_OP_UNINVITE:
        case PARTY_OP_LEAVE:
        case PARTY_OP_SWAP:
            return true;
        default:
            return false;
    }
}

bool IsKnownPartyResult(uint8 result)
{
    switch (PartyResult(result))
    {
        case ERR_PARTY_RESULT_OK:
        case ERR_BAD_PLAYER_NAME_S:
        case ERR_TARGET_NOT_IN_GROUP_S:
        case ERR_TARGET_NOT_IN_INSTANCE_S:
        case ERR_GROUP_FULL:
        case ERR_ALREADY_IN_GROUP_S:
        case ERR_NOT_IN_GROUP:
        case ERR_NOT_LEADER:
        case ERR_PLAYER_WRONG_FACTION:
        case ERR_IGNORING_YOU_S:
        case ERR_LFG_PENDING:
        case ERR_INVITE_RESTRICTED:
        case ERR_GROUP_SWAP_FAILED:
        case ERR_INVITE_UNKNOWN_REALM:
        case ERR_INVITE_NO_PARTY_SERVER:
        case ERR_INVITE_PARTY_BUSY:
        case ERR_PARTY_TARGET_AMBIGUOUS:
        case ERR_PARTY_LFG_INVITE_RAID_LOCKED:
        case ERR_PARTY_LFG_BOOT_LIMIT:
        case ERR_PARTY_LFG_BOOT_COOLDOWN_S:
        case ERR_PARTY_LFG_BOOT_IN_PROGRESS:
        case ERR_PARTY_LFG_BOOT_TOO_FEW_PLAYERS:
        case ERR_PARTY_LFG_BOOT_NOT_ELIGIBLE_S:
        case ERR_RAID_DISALLOWED_BY_LEVEL:
        case ERR_PARTY_LFG_BOOT_IN_COMBAT:
        case ERR_VOTE_KICK_REASON_NEEDED:
        case ERR_PARTY_LFG_BOOT_DUNGEON_COMPLETE:
        case ERR_PARTY_LFG_BOOT_LOOT_ROLLS:
        case ERR_PARTY_LFG_TELEPORT_IN_COMBAT:
            return true;
        default:
            return false;
    }
}

bool TryReadPartyCommandResult(WorldPacket const& packet, PartyCommandResultPayload& out)
{
    PartyCommandResultPayload parsed;
    Result const result = TryReadFully(packet, "SMSG_PARTY_COMMAND_RESULT", [&parsed](WorldPacket& copy)
    {
        ReadPartyCommandResultBody(copy, parsed);
    });

    if (result != Result::Ok)
        return false;

    out = std::move(parsed);
    return true;
}
}
