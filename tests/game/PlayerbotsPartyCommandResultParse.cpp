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

#include "Bot/Packet/BotPacketParse.h"
#include "Bot/Packet/BotPartyCommandResultPacket.h"
#include "ObjectGuid.h"
#include "Opcodes.h"
#include "PartyPackets.h"
#include "SharedDefines.h"
#include "WorldPacket.h"
#include "WorldSession.h"

namespace
{
WorldPacket BuildPartyCommandResultFixture(std::string const& name, uint8 command, uint8 result,
    uint32 resultData = 0)
{
    WorldPackets::Party::PartyCommandResult packet;
    packet.Name = name;
    packet.Command = command;
    packet.Result = result;
    packet.ResultData = resultData;
    packet.ResultGUID = ObjectGuid::Empty;

    WorldPacket const* written = packet.Write();
    WorldPacket copy(*written);
    copy.SetOpcode(SMSG_PARTY_COMMAND_RESULT);
    return copy;
}
}

TEST_CASE("Playerbots PartyCommandResult LEAVE OK parse round-trip (Layer 3 fixture)", "[playerbots][packet]")
{
    WorldPacket packet = BuildPartyCommandResultFixture("Tomma", uint8(PARTY_OP_LEAVE),
        uint8(ERR_PARTY_RESULT_OK));

    Playerbots::PacketParse::PartyCommandResultPayload parsed;
    REQUIRE(Playerbots::PacketParse::TryReadPartyCommandResult(packet, parsed));

    CHECK(parsed.Name == "Tomma");
    CHECK(parsed.Command == uint8(PARTY_OP_LEAVE));
    CHECK(parsed.Result == uint8(ERR_PARTY_RESULT_OK));
    CHECK(parsed.ResultData == 0u);
    CHECK(parsed.ResultGUID.IsEmpty());
    CHECK(Playerbots::PacketParse::IsKnownPartyOperation(parsed.Command));
    CHECK(Playerbots::PacketParse::IsKnownPartyResult(parsed.Result));
    CHECK(Playerbots::PacketParse::VERIFIED_BUILD == 68453u);
}

TEST_CASE("Playerbots PartyCommandResult invite fail parse round-trip (Layer 3 fixture)", "[playerbots][packet]")
{
    WorldPacket packet = BuildPartyCommandResultFixture("Nobody", uint8(PARTY_OP_INVITE),
        uint8(ERR_BAD_PLAYER_NAME_S));

    Playerbots::PacketParse::PartyCommandResultPayload parsed;
    REQUIRE(Playerbots::PacketParse::TryReadPartyCommandResult(packet, parsed));

    CHECK(parsed.Name == "Nobody");
    CHECK(parsed.Command == uint8(PARTY_OP_INVITE));
    CHECK(parsed.Result == uint8(ERR_BAD_PLAYER_NAME_S));
    CHECK(Playerbots::PacketParse::IsKnownPartyOperation(parsed.Command));
    CHECK(Playerbots::PacketParse::IsKnownPartyResult(parsed.Result));
    CHECK(Playerbots::PacketParse::VERIFIED_BUILD == 68453u);
}

TEST_CASE("Playerbots PartyCommandResult truncated payload fails Layer 1 loudly", "[playerbots][packet]")
{
    WorldPacket packet = BuildPartyCommandResultFixture("Blaster", uint8(PARTY_OP_LEAVE),
        uint8(ERR_PARTY_RESULT_OK));
    REQUIRE(packet.size() > 2);
    packet.resize(1);

    Playerbots::PacketParse::PartyCommandResultPayload parsed;
    CHECK_FALSE(Playerbots::PacketParse::TryReadPartyCommandResult(packet, parsed));
}
