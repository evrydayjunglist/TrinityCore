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

#include "tc_catch2.h"

#include "Bot/Packet/BotGuildInvitePacket.h"
#include "Bot/Packet/BotPacketParse.h"
#include "GuildPackets.h"
#include "ObjectGuid.h"
#include "Opcodes.h"
#include "WorldPacket.h"

namespace
{
WorldPacket BuildGuildInviteFixture()
{
    WorldPackets::Guild::GuildInvite invite;
    invite.InviterName = "Inviter";
    invite.GuildName = "TestGuild";
    invite.OldGuildName = "";
    invite.InviterVirtualRealmAddress = 0x01000001;
    invite.GuildVirtualRealmAddress = 0x01000001;
    invite.OldGuildVirtualRealmAddress = 0;
    invite.GuildGUID = ObjectGuid::Create<HighGuid::Guild>(99);
    invite.OldGuildGUID = ObjectGuid::Empty;
    invite.EmblemStyle = 1;
    invite.EmblemColor = 2;
    invite.BorderStyle = 3;
    invite.BorderColor = 4;
    invite.Background = 5;
    invite.AchievementPoints = 1234;

    WorldPacket const* written = invite.Write();
    WorldPacket packet(*written);
    packet.SetOpcode(SMSG_GUILD_INVITE);
    return packet;
}
}

TEST_CASE("Playerbots GuildInvite payload parse round-trip (Layer 3 fixture)", "[playerbots][packet]")
{
    WorldPacket packet = BuildGuildInviteFixture();

    Playerbots::PacketParse::GuildInvitePayload parsed;
    REQUIRE(Playerbots::PacketParse::TryReadGuildInvite(packet, parsed));

    CHECK(parsed.InviterName == "Inviter");
    CHECK(parsed.GuildName == "TestGuild");
    CHECK(parsed.OldGuildName.empty());
    CHECK(parsed.GuildGUID == ObjectGuid::Create<HighGuid::Guild>(99));
    CHECK(parsed.InviterVirtualRealmAddress == 0x01000001u);
    CHECK(parsed.EmblemStyle == 1u);
    CHECK(parsed.EmblemColor == 2u);
    CHECK(parsed.AchievementPoints == 1234);
    CHECK(Playerbots::PacketParse::VERIFIED_BUILD == 68453u);
}

TEST_CASE("Playerbots GuildInvite truncated payload fails Layer 1 loudly", "[playerbots][packet]")
{
    WorldPacket packet = BuildGuildInviteFixture();
    REQUIRE(packet.size() > 4);
    packet.resize(packet.size() / 2);

    Playerbots::PacketParse::GuildInvitePayload parsed;
    CHECK_FALSE(Playerbots::PacketParse::TryReadGuildInvite(packet, parsed));
}
