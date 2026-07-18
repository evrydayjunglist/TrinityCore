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

#include "Bot/Packet/BotBattlefieldStatusPackets.h"
#include "Bot/Packet/BotPacketParse.h"
#include "BattlegroundPackets.h"
#include "LFGPacketsCommon.h"
#include "ObjectGuid.h"
#include "Opcodes.h"
#include "WorldPacket.h"

namespace
{
void FillHeader(WorldPackets::Battleground::BattlefieldStatusHeader& hdr)
{
    hdr.Ticket.RequesterGuid = ObjectGuid::Create<HighGuid::Player>(42);
    hdr.Ticket.Id = 1;
    hdr.Ticket.Type = WorldPackets::LFG::RideType::Battlegrounds;
    hdr.Ticket.Time = time_t(1700000000);
    hdr.Ticket.IsCrossFaction = false;
    hdr.QueueID = { 0x1000000000000001ull };
    hdr.RangeMin = 10;
    hdr.RangeMax = 80;
    hdr.TeamSize = 0;
    hdr.InstanceID = 7;
    hdr.RegisteredMatch = false;
    hdr.TournamentRules = false;
}

template <typename PacketT>
WorldPacket BuildWire(PacketT& packet, uint32 opcode)
{
    WorldPacket const* written = packet.Write();
    WorldPacket copy(*written);
    copy.SetOpcode(opcode);
    return copy;
}
}

TEST_CASE("Playerbots BattlefieldStatusNeedConfirmation parse round-trip (Layer 3 fixture)", "[playerbots][packet]")
{
    WorldPackets::Battleground::BattlefieldStatusNeedConfirmation packet;
    FillHeader(packet.Hdr);
    packet.Mapid = 489;
    packet.Timeout = 120000;
    packet.Role = 1;

    WorldPacket wire = BuildWire(packet, SMSG_BATTLEFIELD_STATUS_NEED_CONFIRMATION);

    Playerbots::PacketParse::BattlefieldStatusNeedConfirmationPayload parsed;
    REQUIRE(Playerbots::PacketParse::TryReadBattlefieldStatusNeedConfirmation(wire, parsed));

    CHECK(parsed.Hdr.Ticket.RequesterGuid == ObjectGuid::Create<HighGuid::Player>(42));
    CHECK(parsed.Hdr.Ticket.Id == 1u);
    CHECK(parsed.Hdr.Ticket.Type == WorldPackets::LFG::RideType::Battlegrounds);
    CHECK(parsed.Hdr.Ticket.Time == time_t(1700000000));
    REQUIRE(parsed.Hdr.QueueID.size() == 1);
    CHECK(parsed.Hdr.QueueID[0] == 0x1000000000000001ull);
    CHECK(parsed.Hdr.RangeMin == 10);
    CHECK(parsed.Hdr.RangeMax == 80);
    CHECK(parsed.Hdr.TeamSize == 0);
    CHECK(parsed.Hdr.InstanceID == 7u);
    CHECK(parsed.Hdr.RegisteredMatch == false);
    CHECK(parsed.Hdr.TournamentRules == false);
    CHECK(parsed.Mapid == 489u);
    CHECK(parsed.Timeout == 120000u);
    CHECK(parsed.Role == 1);
    CHECK(Playerbots::PacketParse::VERIFIED_BUILD == 68453u);
}

TEST_CASE("Playerbots BattlefieldStatusNeedConfirmation truncated payload fails Layer 1 loudly", "[playerbots][packet]")
{
    WorldPackets::Battleground::BattlefieldStatusNeedConfirmation packet;
    FillHeader(packet.Hdr);
    packet.Mapid = 489;
    packet.Timeout = 1;
    packet.Role = 0;

    WorldPacket wire = BuildWire(packet, SMSG_BATTLEFIELD_STATUS_NEED_CONFIRMATION);
    REQUIRE(wire.size() > 8);
    wire.resize(wire.size() / 2);

    Playerbots::PacketParse::BattlefieldStatusNeedConfirmationPayload parsed;
    CHECK_FALSE(Playerbots::PacketParse::TryReadBattlefieldStatusNeedConfirmation(wire, parsed));
}

TEST_CASE("Playerbots BattlefieldStatusQueued parse round-trip (Layer 3 fixture)", "[playerbots][packet]")
{
    WorldPackets::Battleground::BattlefieldStatusQueued packet;
    FillHeader(packet.Hdr);
    packet.AverageWaitTime = 60000;
    packet.WaitTime = 15000;
    packet.SpecSelected = -1;
    packet.AsGroup = true;
    packet.EligibleForMatchmaking = true;
    packet.SuspendedQueue = false;

    WorldPacket wire = BuildWire(packet, SMSG_BATTLEFIELD_STATUS_QUEUED);

    Playerbots::PacketParse::BattlefieldStatusQueuedPayload parsed;
    REQUIRE(Playerbots::PacketParse::TryReadBattlefieldStatusQueued(wire, parsed));

    CHECK(parsed.Hdr.Ticket.Id == 1u);
    REQUIRE(parsed.Hdr.QueueID.size() == 1);
    CHECK(parsed.AverageWaitTime == 60000u);
    CHECK(parsed.WaitTime == 15000u);
    CHECK(parsed.SpecSelected == -1);
    CHECK(parsed.AsGroup == true);
    CHECK(parsed.EligibleForMatchmaking == true);
    CHECK(parsed.SuspendedQueue == false);
    CHECK(Playerbots::PacketParse::VERIFIED_BUILD == 68453u);
}

TEST_CASE("Playerbots BattlefieldStatusQueued truncated payload fails Layer 1 loudly", "[playerbots][packet]")
{
    WorldPackets::Battleground::BattlefieldStatusQueued packet;
    FillHeader(packet.Hdr);
    packet.AverageWaitTime = 1;
    packet.WaitTime = 1;

    WorldPacket wire = BuildWire(packet, SMSG_BATTLEFIELD_STATUS_QUEUED);
    REQUIRE(wire.size() > 8);
    wire.resize(wire.size() / 2);

    Playerbots::PacketParse::BattlefieldStatusQueuedPayload parsed;
    CHECK_FALSE(Playerbots::PacketParse::TryReadBattlefieldStatusQueued(wire, parsed));
}

TEST_CASE("Playerbots BattlefieldStatusActive parse round-trip (Layer 3 fixture)", "[playerbots][packet]")
{
    WorldPackets::Battleground::BattlefieldStatusActive packet;
    FillHeader(packet.Hdr);
    packet.Mapid = 489;
    packet.ShutdownTimer = 0;
    packet.StartTimer = 30000;
    packet.ArenaFaction = 0;
    packet.LeftEarly = false;
    packet.Brawl = true;

    WorldPacket wire = BuildWire(packet, SMSG_BATTLEFIELD_STATUS_ACTIVE);

    Playerbots::PacketParse::BattlefieldStatusActivePayload parsed;
    REQUIRE(Playerbots::PacketParse::TryReadBattlefieldStatusActive(wire, parsed));

    CHECK(parsed.Hdr.Ticket.Id == 1u);
    REQUIRE(parsed.Hdr.QueueID.size() == 1);
    CHECK(parsed.Mapid == 489u);
    CHECK(parsed.ShutdownTimer == 0u);
    CHECK(parsed.StartTimer == 30000u);
    CHECK(parsed.ArenaFaction == 0);
    CHECK(parsed.LeftEarly == false);
    CHECK(parsed.Brawl == true);
    CHECK(Playerbots::PacketParse::VERIFIED_BUILD == 68453u);
}

TEST_CASE("Playerbots BattlefieldStatusActive truncated payload fails Layer 1 loudly", "[playerbots][packet]")
{
    WorldPackets::Battleground::BattlefieldStatusActive packet;
    FillHeader(packet.Hdr);
    packet.Mapid = 489;

    WorldPacket wire = BuildWire(packet, SMSG_BATTLEFIELD_STATUS_ACTIVE);
    REQUIRE(wire.size() > 8);
    wire.resize(wire.size() / 2);

    Playerbots::PacketParse::BattlefieldStatusActivePayload parsed;
    CHECK_FALSE(Playerbots::PacketParse::TryReadBattlefieldStatusActive(wire, parsed));
}
