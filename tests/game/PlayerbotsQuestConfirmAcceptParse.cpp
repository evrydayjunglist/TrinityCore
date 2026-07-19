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
#include "Bot/Packet/BotQuestConfirmAcceptPacket.h"
#include "ObjectGuid.h"
#include "Opcodes.h"
#include "QuestPackets.h"
#include "WorldPacket.h"

namespace
{
WorldPacket BuildQuestConfirmAcceptFixture(std::string title)
{
    WorldPackets::Quest::QuestConfirmAcceptResponse packet;
    packet.QuestID = 28725;
    packet.InitiatedBy = ObjectGuid::Create<HighGuid::Player>(42);
    packet.QuestTitle = std::move(title);

    WorldPacket const* written = packet.Write();
    WorldPacket copy(*written);
    copy.SetOpcode(SMSG_QUEST_CONFIRM_ACCEPT);
    return copy;
}
}

TEST_CASE("Playerbots QuestConfirmAccept non-empty title parse round-trip (Layer 3 fixture)", "[playerbots][packet]")
{
    WorldPacket wire = BuildQuestConfirmAcceptFixture("Shared Quest Title");

    Playerbots::PacketParse::QuestConfirmAcceptPayload parsed;
    REQUIRE(Playerbots::PacketParse::TryReadQuestConfirmAccept(wire, parsed));

    CHECK(parsed.QuestID == 28725u);
    CHECK(parsed.InitiatedBy == ObjectGuid::Create<HighGuid::Player>(42));
    CHECK(parsed.QuestTitle == "Shared Quest Title");
    CHECK(Playerbots::PacketParse::VERIFIED_BUILD == 68453u);
}

TEST_CASE("Playerbots QuestConfirmAccept truncated payload fails Layer 1 loudly", "[playerbots][packet]")
{
    WorldPacket wire = BuildQuestConfirmAcceptFixture("Title");
    REQUIRE(wire.size() > 8);
    wire.resize(wire.size() / 2);

    Playerbots::PacketParse::QuestConfirmAcceptPayload parsed;
    CHECK_FALSE(Playerbots::PacketParse::TryReadQuestConfirmAccept(wire, parsed));
}
