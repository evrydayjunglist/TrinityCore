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
#include "Bot/Packet/BotQuestUpdateAddCreditPacket.h"
#include "ObjectGuid.h"
#include "Opcodes.h"
#include "QuestPackets.h"
#include "WorldPacket.h"

namespace
{
WorldPacket BuildQuestUpdateAddCreditFixture()
{
    WorldPackets::Quest::QuestUpdateAddCredit packet;
    packet.VictimGUID = ObjectGuid::Create<HighGuid::Creature>(0, 3100, 1001);
    packet.QuestID = 2600;
    packet.ObjectID = 3100;
    packet.Count = 3;
    packet.Required = 10;
    packet.ObjectiveType = 0; // QUEST_OBJECTIVE_MONSTER

    WorldPacket const* written = packet.Write();
    WorldPacket copy(*written);
    copy.SetOpcode(SMSG_QUEST_UPDATE_ADD_CREDIT);
    return copy;
}
}

TEST_CASE("Playerbots QuestUpdateAddCredit parse round-trip (Layer 3 fixture)", "[playerbots][packet]")
{
    WorldPacket wire = BuildQuestUpdateAddCreditFixture();

    Playerbots::PacketParse::QuestUpdateAddCreditPayload parsed;
    REQUIRE(Playerbots::PacketParse::TryReadQuestUpdateAddCredit(wire, parsed));

    CHECK(parsed.VictimGUID == ObjectGuid::Create<HighGuid::Creature>(0, 3100, 1001));
    CHECK(parsed.QuestID == 2600);
    CHECK(parsed.ObjectID == 3100);
    CHECK(parsed.Count == 3);
    CHECK(parsed.Required == 10);
    CHECK(parsed.ObjectiveType == 0u);
    CHECK(Playerbots::PacketParse::VERIFIED_BUILD == 68453u);
}

TEST_CASE("Playerbots QuestUpdateAddCredit truncated payload fails Layer 1 loudly", "[playerbots][packet]")
{
    WorldPacket wire = BuildQuestUpdateAddCreditFixture();
    REQUIRE(wire.size() > 8);
    wire.resize(wire.size() / 2);

    Playerbots::PacketParse::QuestUpdateAddCreditPayload parsed;
    CHECK_FALSE(Playerbots::PacketParse::TryReadQuestUpdateAddCredit(wire, parsed));
}
