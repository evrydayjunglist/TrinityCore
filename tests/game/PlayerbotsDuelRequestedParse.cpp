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

#include "Bot/Packet/BotDuelRequestedPacket.h"
#include "Bot/Packet/BotPacketParse.h"
#include "DuelPackets.h"
#include "ObjectGuid.h"
#include "Opcodes.h"
#include "WorldPacket.h"

namespace
{
WorldPacket BuildDuelRequestedFixture(WorldPackets::Duel::DuelRequested& packet)
{
    WorldPacket const* written = packet.Write();
    WorldPacket copy(*written);
    copy.SetOpcode(SMSG_DUEL_REQUESTED);
    return copy;
}
}

TEST_CASE("Playerbots DuelRequested ToTheDeath=false parse round-trip (Layer 3 fixture)", "[playerbots][packet]")
{
    WorldPackets::Duel::DuelRequested packet;
    packet.ArbiterGUID = ObjectGuid::Create<HighGuid::GameObject>(1, 100, 1);
    packet.RequestedByGUID = ObjectGuid::Create<HighGuid::Player>(42);
    packet.RequestedByWowAccount = ObjectGuid::Create<HighGuid::WowAccount>(7);
    packet.ToTheDeath = false;

    WorldPacket wire = BuildDuelRequestedFixture(packet);

    Playerbots::PacketParse::DuelRequestedPayload parsed;
    REQUIRE(Playerbots::PacketParse::TryReadDuelRequested(wire, parsed));

    CHECK(parsed.ArbiterGUID == ObjectGuid::Create<HighGuid::GameObject>(1, 100, 1));
    CHECK(parsed.RequestedByGUID == ObjectGuid::Create<HighGuid::Player>(42));
    CHECK(parsed.RequestedByWowAccount == ObjectGuid::Create<HighGuid::WowAccount>(7));
    CHECK(parsed.ToTheDeath == false);
    CHECK(Playerbots::PacketParse::VERIFIED_BUILD == 68453u);
}

TEST_CASE("Playerbots DuelRequested ToTheDeath=true parse round-trip (Layer 3 fixture)", "[playerbots][packet]")
{
    WorldPackets::Duel::DuelRequested packet;
    packet.ArbiterGUID = ObjectGuid::Create<HighGuid::GameObject>(2, 200, 3);
    packet.RequestedByGUID = ObjectGuid::Create<HighGuid::Player>(9);
    packet.RequestedByWowAccount = ObjectGuid::Create<HighGuid::WowAccount>(11);
    packet.ToTheDeath = true;

    WorldPacket wire = BuildDuelRequestedFixture(packet);

    Playerbots::PacketParse::DuelRequestedPayload parsed;
    REQUIRE(Playerbots::PacketParse::TryReadDuelRequested(wire, parsed));

    CHECK(parsed.ArbiterGUID == ObjectGuid::Create<HighGuid::GameObject>(2, 200, 3));
    CHECK(parsed.RequestedByGUID == ObjectGuid::Create<HighGuid::Player>(9));
    CHECK(parsed.RequestedByWowAccount == ObjectGuid::Create<HighGuid::WowAccount>(11));
    CHECK(parsed.ToTheDeath == true);
    CHECK(Playerbots::PacketParse::VERIFIED_BUILD == 68453u);
}

TEST_CASE("Playerbots DuelRequested truncated payload fails Layer 1 loudly", "[playerbots][packet]")
{
    WorldPackets::Duel::DuelRequested packet;
    packet.ArbiterGUID = ObjectGuid::Create<HighGuid::GameObject>(1, 1, 1);
    packet.RequestedByGUID = ObjectGuid::Create<HighGuid::Player>(1);
    packet.RequestedByWowAccount = ObjectGuid::Create<HighGuid::WowAccount>(1);
    packet.ToTheDeath = false;

    WorldPacket wire = BuildDuelRequestedFixture(packet);
    REQUIRE(wire.size() > 4);
    wire.resize(4);

    Playerbots::PacketParse::DuelRequestedPayload parsed;
    CHECK_FALSE(Playerbots::PacketParse::TryReadDuelRequested(wire, parsed));
}
