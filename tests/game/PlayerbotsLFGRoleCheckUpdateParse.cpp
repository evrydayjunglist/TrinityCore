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

#include "Bot/Packet/BotLFGRoleCheckUpdatePacket.h"
#include "Bot/Packet/BotPacketParse.h"
#include "LFGMgr.h"
#include "LFGPackets.h"
#include "ObjectGuid.h"
#include "Opcodes.h"
#include "WorldPacket.h"

namespace
{
WorldPacket BuildLFGRoleCheckUpdateFixture(WorldPackets::LFG::LFGRoleCheckUpdate& packet)
{
    WorldPacket const* written = packet.Write();
    WorldPacket copy(*written);
    copy.SetOpcode(SMSG_LFG_ROLE_CHECK_UPDATE);
    return copy;
}
}

TEST_CASE("Playerbots LFGRoleCheckUpdate multi-member parse round-trip (Layer 3 fixture)", "[playerbots][packet]")
{
    WorldPackets::LFG::LFGRoleCheckUpdate packet;
    packet.PartyIndex = 127;
    packet.RoleCheckStatus = uint8(lfg::LFG_ROLECHECK_INITIALITING);
    packet.JoinSlots = { 462, 463 };
    packet.BgQueueIDs = { };
    packet.GroupFinderActivityID = 0;
    packet.IsBeginning = true;
    packet.IsRequeue = false;
    packet.Members = {
        { ObjectGuid::Create<HighGuid::Player>(1), uint8(lfg::PLAYER_ROLE_TANK), 80, false },
        { ObjectGuid::Create<HighGuid::Player>(2), uint8(lfg::PLAYER_ROLE_DAMAGE), 80, true },
    };

    WorldPacket wire = BuildLFGRoleCheckUpdateFixture(packet);

    Playerbots::PacketParse::LFGRoleCheckUpdatePayload parsed;
    REQUIRE(Playerbots::PacketParse::TryReadLFGRoleCheckUpdate(wire, parsed));

    CHECK(parsed.PartyIndex == 127);
    CHECK(parsed.RoleCheckStatus == uint8(lfg::LFG_ROLECHECK_INITIALITING));
    REQUIRE(parsed.JoinSlots.size() == 2);
    CHECK(parsed.JoinSlots[0] == 462u);
    CHECK(parsed.JoinSlots[1] == 463u);
    CHECK(parsed.BgQueueIDs.empty());
    CHECK(parsed.GroupFinderActivityID == 0);
    CHECK(parsed.IsBeginning == true);
    CHECK(parsed.IsRequeue == false);
    REQUIRE(parsed.Members.size() == 2);
    CHECK(parsed.Members[0].Guid == ObjectGuid::Create<HighGuid::Player>(1));
    CHECK(parsed.Members[0].RolesDesired == uint8(lfg::PLAYER_ROLE_TANK));
    CHECK(parsed.Members[0].Level == 80);
    CHECK(parsed.Members[0].RoleCheckComplete == false);
    CHECK(parsed.Members[1].Guid == ObjectGuid::Create<HighGuid::Player>(2));
    CHECK(parsed.Members[1].RolesDesired == uint8(lfg::PLAYER_ROLE_DAMAGE));
    CHECK(parsed.Members[1].RoleCheckComplete == true);
    CHECK(Playerbots::PacketParse::VERIFIED_BUILD == 68453u);
}

TEST_CASE("Playerbots LFGRoleCheckUpdate truncated payload fails Layer 1 loudly", "[playerbots][packet]")
{
    WorldPackets::LFG::LFGRoleCheckUpdate packet;
    packet.PartyIndex = 127;
    packet.RoleCheckStatus = uint8(lfg::LFG_ROLECHECK_INITIALITING);
    packet.JoinSlots = { 462 };
    packet.Members = {
        { ObjectGuid::Create<HighGuid::Player>(1), uint8(lfg::PLAYER_ROLE_DAMAGE), 70, false },
    };
    packet.IsBeginning = true;

    WorldPacket wire = BuildLFGRoleCheckUpdateFixture(packet);
    REQUIRE(wire.size() > 8);
    wire.resize(wire.size() / 2);

    Playerbots::PacketParse::LFGRoleCheckUpdatePayload parsed;
    CHECK_FALSE(Playerbots::PacketParse::TryReadLFGRoleCheckUpdate(wire, parsed));
}
