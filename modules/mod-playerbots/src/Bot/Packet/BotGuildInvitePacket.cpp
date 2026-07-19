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

#include "BotGuildInvitePacket.h"
#include "BotPacketParse.h"
#include "PacketOperators.h"

namespace Playerbots::PacketParse
{
namespace
{
void ReadGuildInviteBody(ByteBuffer& data, GuildInvitePayload& out)
{
    // Mirror WorldPackets::Guild::GuildInvite::Write() field order exactly.
    data >> WorldPackets::SizedString::BitsSize<6>(out.InviterName);
    data >> WorldPackets::SizedString::BitsSize<7>(out.GuildName);
    data >> WorldPackets::SizedString::BitsSize<7>(out.OldGuildName);
    data.ResetBitPos();

    data >> out.InviterVirtualRealmAddress;
    data >> out.GuildVirtualRealmAddress;
    data >> out.GuildGUID;
    data >> out.OldGuildVirtualRealmAddress;
    data >> out.OldGuildGUID;
    data >> out.EmblemStyle;
    data >> out.EmblemColor;
    data >> out.BorderStyle;
    data >> out.BorderColor;
    data >> out.Background;
    data >> out.AchievementPoints;

    data >> WorldPackets::SizedString::Data(out.InviterName);
    data >> WorldPackets::SizedString::Data(out.GuildName);
    data >> WorldPackets::SizedString::Data(out.OldGuildName);
}
}

bool TryReadGuildInvite(WorldPacket const& packet, GuildInvitePayload& out)
{
    GuildInvitePayload parsed;
    Result const result = TryReadFully(packet, "SMSG_GUILD_INVITE", [&parsed](WorldPacket& copy)
    {
        ReadGuildInviteBody(copy, parsed);
    });

    if (result != Result::Ok)
        return false;

    out = std::move(parsed);
    return true;
}
}
