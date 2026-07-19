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
#include "Bot/Packet/BotQuestUpdateCompletePacket.h"
#include "Opcodes.h"
#include "QuestPackets.h"
#include "WorldPacket.h"

namespace
{
WorldPacket BuildQuestUpdateCompleteFixture(int32 questId, bool hideCreditMessage)
{
    WorldPackets::Quest::QuestUpdateComplete packet;
    packet.QuestID = questId;
    packet.HideCreditMessage = hideCreditMessage;

    WorldPacket const* written = packet.Write();
    WorldPacket copy(*written);
    copy.SetOpcode(SMSG_QUEST_UPDATE_COMPLETE);
    return copy;
}
}

TEST_CASE("Playerbots QuestUpdateComplete parse round-trip (Layer 3 fixture)", "[playerbots][packet]")
{
    WorldPacket wire = BuildQuestUpdateCompleteFixture(12345, false);

    Playerbots::PacketParse::QuestUpdateCompletePayload parsed;
    REQUIRE(Playerbots::PacketParse::TryReadQuestUpdateComplete(wire, parsed));

    CHECK(parsed.QuestID == 12345);
    CHECK(parsed.HideCreditMessage == false);
    CHECK(Playerbots::PacketParse::VERIFIED_BUILD == 68453u);
}

TEST_CASE("Playerbots QuestUpdateComplete HideCreditMessage=true parse round-trip (Layer 3 fixture)", "[playerbots][packet]")
{
    WorldPacket wire = BuildQuestUpdateCompleteFixture(99, true);

    Playerbots::PacketParse::QuestUpdateCompletePayload parsed;
    REQUIRE(Playerbots::PacketParse::TryReadQuestUpdateComplete(wire, parsed));

    CHECK(parsed.QuestID == 99);
    CHECK(parsed.HideCreditMessage == true);
}

TEST_CASE("Playerbots QuestUpdateComplete truncated payload fails Layer 1 loudly", "[playerbots][packet]")
{
    WorldPacket wire = BuildQuestUpdateCompleteFixture(1, false);
    REQUIRE(wire.size() > 2);
    wire.resize(2);

    Playerbots::PacketParse::QuestUpdateCompletePayload parsed;
    CHECK_FALSE(Playerbots::PacketParse::TryReadQuestUpdateComplete(wire, parsed));
}
