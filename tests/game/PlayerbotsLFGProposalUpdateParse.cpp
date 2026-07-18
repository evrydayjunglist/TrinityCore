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

#include "Bot/Packet/BotLFGProposalUpdatePacket.h"
#include "Bot/Packet/BotPacketParse.h"
#include "LFG.h"
#include "LFGMgr.h"
#include "LFGPackets.h"
#include "ObjectGuid.h"
#include "Opcodes.h"
#include "WorldPacket.h"

namespace
{
WorldPacket BuildLFGProposalUpdateFixture(WorldPackets::LFG::LFGProposalUpdate& packet)
{
    WorldPacket const* written = packet.Write();
    WorldPacket copy(*written);
    copy.SetOpcode(SMSG_LFG_PROPOSAL_UPDATE);
    return copy;
}
}

TEST_CASE("Playerbots LFGProposalUpdate multi-player parse round-trip (Layer 3 fixture)", "[playerbots][packet]")
{
    WorldPackets::LFG::LFGProposalUpdate packet;
    packet.Ticket.RequesterGuid = ObjectGuid::Create<HighGuid::Player>(42);
    packet.Ticket.Id = 7;
    packet.Ticket.Type = WorldPackets::LFG::RideType::Lfg;
    packet.Ticket.Time = time_t(1700000000);
    packet.Ticket.IsCrossFaction = false;
    packet.InstanceID = 0;
    packet.ProposalID = 12345;
    packet.Slot = 462;
    packet.State = int8(lfg::LFG_PROPOSAL_INITIATING);
    packet.CompletedMask = 0;
    packet.EncounterMask = 0;
    packet.PromisedShortageRolePriority = 0;
    packet.ValidCompletedMask = true;
    packet.ProposalSilent = false;
    packet.FailedByMyParty = false;

    WorldPackets::LFG::LFGProposalUpdatePlayer self;
    self.Roles = uint8(lfg::PLAYER_ROLE_DAMAGE);
    self.Me = true;
    self.SameParty = true;
    self.MyParty = true;
    self.Responded = false;
    self.Accepted = false;

    WorldPackets::LFG::LFGProposalUpdatePlayer other;
    other.Roles = uint8(lfg::PLAYER_ROLE_TANK);
    other.Me = false;
    other.SameParty = true;
    other.MyParty = false;
    other.Responded = true;
    other.Accepted = true;

    packet.Players = { self, other };

    WorldPacket wire = BuildLFGProposalUpdateFixture(packet);

    Playerbots::PacketParse::LFGProposalUpdatePayload parsed;
    REQUIRE(Playerbots::PacketParse::TryReadLFGProposalUpdate(wire, parsed));

    CHECK(parsed.Ticket.RequesterGuid == ObjectGuid::Create<HighGuid::Player>(42));
    CHECK(parsed.Ticket.Id == 7u);
    CHECK(parsed.Ticket.Type == WorldPackets::LFG::RideType::Lfg);
    CHECK(parsed.Ticket.Time == time_t(1700000000));
    CHECK(parsed.Ticket.IsCrossFaction == false);
    CHECK(parsed.InstanceID == 0u);
    CHECK(parsed.ProposalID == 12345u);
    CHECK(parsed.Slot == 462u);
    CHECK(parsed.State == int8(lfg::LFG_PROPOSAL_INITIATING));
    CHECK(parsed.ValidCompletedMask == true);
    CHECK(parsed.ProposalSilent == false);
    CHECK(parsed.FailedByMyParty == false);
    REQUIRE(parsed.Players.size() == 2);
    CHECK(parsed.Players[0].Roles == uint8(lfg::PLAYER_ROLE_DAMAGE));
    CHECK(parsed.Players[0].Me == true);
    CHECK(parsed.Players[0].Responded == false);
    CHECK(parsed.Players[1].Roles == uint8(lfg::PLAYER_ROLE_TANK));
    CHECK(parsed.Players[1].Me == false);
    CHECK(parsed.Players[1].Accepted == true);
    CHECK(Playerbots::PacketParse::VERIFIED_BUILD == 68453u);
}

TEST_CASE("Playerbots LFGProposalUpdate truncated payload fails Layer 1 loudly", "[playerbots][packet]")
{
    WorldPackets::LFG::LFGProposalUpdate packet;
    packet.Ticket.RequesterGuid = ObjectGuid::Create<HighGuid::Player>(1);
    packet.Ticket.Id = 1;
    packet.Ticket.Type = WorldPackets::LFG::RideType::Lfg;
    packet.Ticket.Time = time_t(1);
    packet.ProposalID = 9;
    packet.Slot = 1;
    packet.Players = { {} };

    WorldPacket wire = BuildLFGProposalUpdateFixture(packet);
    REQUIRE(wire.size() > 8);
    wire.resize(wire.size() / 2);

    Playerbots::PacketParse::LFGProposalUpdatePayload parsed;
    CHECK_FALSE(Playerbots::PacketParse::TryReadLFGProposalUpdate(wire, parsed));
}
