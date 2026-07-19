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

#include "BotLFGRoleCheckUpdatePacket.h"
#include "BotPacketParse.h"
#include "PacketOperators.h"

namespace Playerbots::PacketParse
{
namespace
{
void ReadLFGRoleCheckUpdateMember(ByteBuffer& data, LFGRoleCheckUpdateMemberPayload& out)
{
    // Mirror WorldPackets::LFG::operator<<(LFGRoleCheckUpdateMember) field order exactly.
    data >> out.Guid;
    data >> out.RolesDesired;
    data >> out.Level;
    data >> WorldPackets::Bits<1>(out.RoleCheckComplete);
    data.ResetBitPos();
}

void ReadLFGRoleCheckUpdateBody(ByteBuffer& data, LFGRoleCheckUpdatePayload& out)
{
    // Mirror WorldPackets::LFG::LFGRoleCheckUpdate::Write() field order exactly.
    // Sizes for JoinSlots / BgQueueIDs / Members precede *all* JoinSlots/BgQueueIDs data;
    // IsBeginning/IsRequeue bits come after those payloads; Members come last.
    data >> out.PartyIndex;
    data >> out.RoleCheckStatus;
    data >> WorldPackets::Size<uint32>(out.JoinSlots);
    data >> WorldPackets::Size<uint32>(out.BgQueueIDs);
    data >> out.GroupFinderActivityID;
    data >> WorldPackets::Size<uint32>(out.Members);

    for (uint32& slot : out.JoinSlots)
        data >> slot;

    for (uint64& bgQueueID : out.BgQueueIDs)
        data >> bgQueueID;

    data >> WorldPackets::Bits<1>(out.IsBeginning);
    data >> WorldPackets::Bits<1>(out.IsRequeue);
    data.ResetBitPos();

    for (LFGRoleCheckUpdateMemberPayload& member : out.Members)
        ReadLFGRoleCheckUpdateMember(data, member);
}
}

bool TryReadLFGRoleCheckUpdate(WorldPacket const& packet, LFGRoleCheckUpdatePayload& out)
{
    LFGRoleCheckUpdatePayload parsed;
    Result const result = TryReadFully(packet, "SMSG_LFG_ROLE_CHECK_UPDATE", [&parsed](WorldPacket& copy)
    {
        ReadLFGRoleCheckUpdateBody(copy, parsed);
    });

    if (result != Result::Ok)
        return false;

    out = std::move(parsed);
    return true;
}
}
