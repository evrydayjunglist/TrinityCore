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

#include "BotGroupNewLeaderPacket.h"
#include "BotPacketParse.h"
#include "PacketOperators.h"

namespace Playerbots::PacketParse
{
namespace
{
void ReadGroupNewLeaderBody(ByteBuffer& data, GroupNewLeaderPayload& out)
{
    // Mirror WorldPackets::Party::GroupNewLeader::Write() field order exactly.
    data >> out.PartyIndex;
    data >> WorldPackets::SizedString::BitsSize<9>(out.Name);
    data.ResetBitPos();
    data >> WorldPackets::SizedString::Data(out.Name);
}
}

bool TryReadGroupNewLeader(WorldPacket const& packet, GroupNewLeaderPayload& out)
{
    GroupNewLeaderPayload parsed;
    Result const result = TryReadFully(packet, "SMSG_GROUP_NEW_LEADER", [&parsed](WorldPacket& copy)
    {
        ReadGroupNewLeaderBody(copy, parsed);
    });

    if (result != Result::Ok)
        return false;

    out = std::move(parsed);
    return true;
}
}
