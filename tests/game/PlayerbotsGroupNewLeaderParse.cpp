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

#include "Bot/Packet/BotGroupNewLeaderPacket.h"
#include "Bot/Packet/BotPacketParse.h"
#include "Opcodes.h"
#include "PartyPackets.h"
#include "WorldPacket.h"

namespace
{
WorldPacket BuildGroupNewLeaderFixture(int8 partyIndex, std::string const& name)
{
    WorldPackets::Party::GroupNewLeader packet;
    packet.PartyIndex = partyIndex;
    packet.Name = name;

    WorldPacket const* written = packet.Write();
    WorldPacket copy(*written);
    copy.SetOpcode(SMSG_GROUP_NEW_LEADER);
    return copy;
}
}

TEST_CASE("Playerbots GroupNewLeader payload parse round-trip (Layer 3 fixture)", "[playerbots][packet]")
{
    WorldPacket packet = BuildGroupNewLeaderFixture(0, "Blaster");

    Playerbots::PacketParse::GroupNewLeaderPayload parsed;
    REQUIRE(Playerbots::PacketParse::TryReadGroupNewLeader(packet, parsed));

    CHECK(parsed.PartyIndex == 0);
    CHECK(parsed.Name == "Blaster");
    CHECK(Playerbots::PacketParse::VERIFIED_BUILD == 68453u);
}

TEST_CASE("Playerbots GroupNewLeader instance-category payload parse round-trip (Layer 3 fixture)", "[playerbots][packet]")
{
    WorldPacket packet = BuildGroupNewLeaderFixture(1, "Tomma");

    Playerbots::PacketParse::GroupNewLeaderPayload parsed;
    REQUIRE(Playerbots::PacketParse::TryReadGroupNewLeader(packet, parsed));

    CHECK(parsed.PartyIndex == 1);
    CHECK(parsed.Name == "Tomma");
    CHECK(Playerbots::PacketParse::VERIFIED_BUILD == 68453u);
}

TEST_CASE("Playerbots GroupNewLeader truncated payload fails Layer 1 loudly", "[playerbots][packet]")
{
    WorldPacket packet = BuildGroupNewLeaderFixture(0, "Blaster");
    REQUIRE(packet.size() > 2);
    packet.resize(1);

    Playerbots::PacketParse::GroupNewLeaderPayload parsed;
    CHECK_FALSE(Playerbots::PacketParse::TryReadGroupNewLeader(packet, parsed));
}
