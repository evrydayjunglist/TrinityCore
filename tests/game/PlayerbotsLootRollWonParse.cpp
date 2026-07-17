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

#include "Bot/Packet/BotLootRollWonPacket.h"
#include "Bot/Packet/BotPacketParse.h"
#include "Loot.h"
#include "LootItemType.h"
#include "LootPackets.h"
#include "ObjectGuid.h"
#include "Opcodes.h"
#include "Util.h"
#include "WorldPacket.h"

namespace
{
WorldPacket BuildLootRollWonFixture(WorldPackets::Loot::LootRollWon& packet)
{
    WorldPacket const* written = packet.Write();
    WorldPacket copy(*written);
    copy.SetOpcode(SMSG_LOOT_ROLL_WON);
    return copy;
}
}

TEST_CASE("Playerbots LootRollWon winner ALLOW_LOOT parse round-trip (Layer 3 fixture)", "[playerbots][packet]")
{
    WorldPackets::Loot::LootRollWon packet;
    packet.LootObj = ObjectGuid::Create<HighGuid::LootObject>(0, 0, 200);
    packet.Winner = ObjectGuid::Create<HighGuid::Player>(42);
    packet.Roll = 87;
    packet.RollType = AsUnderlyingType(RollVote::Need);
    packet.DungeonEncounterID = 0;
    packet.MainSpec = true;
    packet.Item.Type = LootItemType::Item;
    packet.Item.UIType = LOOT_SLOT_TYPE_ALLOW_LOOT;
    packet.Item.CanTradeToTapList = false;
    packet.Item.Loot.ItemID = 6948;
    packet.Item.Quantity = 1;
    packet.Item.LootItemType = 0;
    packet.Item.LootListID = 3;

    WorldPacket wire = BuildLootRollWonFixture(packet);

    Playerbots::PacketParse::LootRollWonPayload parsed;
    REQUIRE(Playerbots::PacketParse::TryReadLootRollWon(wire, parsed));

    CHECK(parsed.LootObj == ObjectGuid::Create<HighGuid::LootObject>(0, 0, 200));
    CHECK(parsed.Winner == ObjectGuid::Create<HighGuid::Player>(42));
    CHECK(parsed.Roll == 87);
    CHECK(parsed.RollType == AsUnderlyingType(RollVote::Need));
    CHECK(parsed.DungeonEncounterID == 0);
    CHECK(parsed.MainSpec == true);
    CHECK(parsed.Item.Type == LootItemType::Item);
    CHECK(parsed.Item.UIType == uint8(LOOT_SLOT_TYPE_ALLOW_LOOT));
    CHECK(parsed.Item.CanTradeToTapList == false);
    CHECK(parsed.Item.Loot.ItemID == 6948u);
    CHECK(parsed.Item.Quantity == 1u);
    CHECK(parsed.Item.LootListID == 3);
    CHECK(Playerbots::PacketParse::IsKnownLootRollWonRollType(parsed.RollType));
    CHECK(Playerbots::PacketParse::VERIFIED_BUILD == 68453u);
}

TEST_CASE("Playerbots LootRollWon non-winner LOCKED parse round-trip (Layer 3 fixture)", "[playerbots][packet]")
{
    WorldPackets::Loot::LootRollWon packet;
    packet.LootObj = ObjectGuid::Create<HighGuid::LootObject>(0, 0, 66);
    packet.Winner = ObjectGuid::Create<HighGuid::Player>(7);
    packet.Roll = 12;
    packet.RollType = AsUnderlyingType(RollVote::Greed);
    packet.MainSpec = true;
    packet.Item.Type = LootItemType::Item;
    packet.Item.UIType = LOOT_SLOT_TYPE_LOCKED;
    packet.Item.Loot.ItemID = 2589;
    packet.Item.Quantity = 2;
    packet.Item.LootListID = 1;

    WorldPacket wire = BuildLootRollWonFixture(packet);

    Playerbots::PacketParse::LootRollWonPayload parsed;
    REQUIRE(Playerbots::PacketParse::TryReadLootRollWon(wire, parsed));

    CHECK(parsed.RollType == AsUnderlyingType(RollVote::Greed));
    CHECK(parsed.Item.UIType == uint8(LOOT_SLOT_TYPE_LOCKED));
    CHECK(parsed.Item.Loot.ItemID == 2589u);
    CHECK(parsed.Item.Quantity == 2u);
    CHECK(Playerbots::PacketParse::VERIFIED_BUILD == 68453u);
}

TEST_CASE("Playerbots LootRollWon truncated payload fails Layer 1 loudly", "[playerbots][packet]")
{
    WorldPackets::Loot::LootRollWon packet;
    packet.LootObj = ObjectGuid::Create<HighGuid::LootObject>(0, 0, 2);
    packet.Winner = ObjectGuid::Create<HighGuid::Player>(1);
    packet.Roll = 50;
    packet.RollType = AsUnderlyingType(RollVote::Need);
    packet.MainSpec = true;
    packet.Item.Type = LootItemType::Item;
    packet.Item.UIType = LOOT_SLOT_TYPE_ALLOW_LOOT;
    packet.Item.Loot.ItemID = 12345;
    packet.Item.Quantity = 1;
    packet.Item.LootListID = 1;

    WorldPacket wire = BuildLootRollWonFixture(packet);
    REQUIRE(wire.size() > 8);
    wire.resize(wire.size() / 2);

    Playerbots::PacketParse::LootRollWonPayload parsed;
    CHECK_FALSE(Playerbots::PacketParse::TryReadLootRollWon(wire, parsed));
}
