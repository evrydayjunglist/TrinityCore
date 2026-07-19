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
#include "Bot/Packet/BotStartLootRollPacket.h"
#include "Loot.h"
#include "LootItemType.h"
#include "LootPackets.h"
#include "ObjectGuid.h"
#include "Opcodes.h"
#include "WorldPacket.h"

namespace
{
WorldPacket BuildStartLootRollFixture(WorldPackets::Loot::StartLootRoll& packet)
{
    WorldPacket const* written = packet.Write();
    WorldPacket copy(*written);
    copy.SetOpcode(SMSG_START_LOOT_ROLL);
    return copy;
}
}

TEST_CASE("Playerbots StartLootRoll GROUP_LOOT parse round-trip (Layer 3 fixture)", "[playerbots][packet]")
{
    WorldPackets::Loot::StartLootRoll packet;
    packet.LootObj = ObjectGuid::Create<HighGuid::LootObject>(0, 0, 200);
    packet.MapID = 0;
    packet.RollTime = 60000ms;
    packet.ValidRolls = ROLL_ALL_TYPE_NO_DISENCHANT;
    packet.LootRollIneligibleReason = { };
    packet.Method = GROUP_LOOT;
    packet.DungeonEncounterID = 0;
    packet.Item.Type = LootItemType::Item;
    packet.Item.UIType = LOOT_SLOT_TYPE_ROLL_ONGOING;
    packet.Item.CanTradeToTapList = false;
    packet.Item.Loot.ItemID = 6948;
    packet.Item.Quantity = 1;
    packet.Item.LootItemType = 0;
    packet.Item.LootListID = 3;

    WorldPacket wire = BuildStartLootRollFixture(packet);

    Playerbots::PacketParse::StartLootRollPayload parsed;
    REQUIRE(Playerbots::PacketParse::TryReadStartLootRoll(wire, parsed));

    CHECK(parsed.LootObj == ObjectGuid::Create<HighGuid::LootObject>(0, 0, 200));
    CHECK(parsed.MapID == 0);
    CHECK(parsed.RollTime == 60000u);
    CHECK(parsed.ValidRolls == uint8(ROLL_ALL_TYPE_NO_DISENCHANT));
    CHECK(parsed.Method == uint8(GROUP_LOOT));
    CHECK(parsed.DungeonEncounterID == 0);
    CHECK(parsed.Item.Type == LootItemType::Item);
    CHECK(parsed.Item.UIType == uint8(LOOT_SLOT_TYPE_ROLL_ONGOING));
    CHECK(parsed.Item.CanTradeToTapList == false);
    CHECK(parsed.Item.Loot.ItemID == 6948u);
    CHECK(parsed.Item.Quantity == 1u);
    CHECK(parsed.Item.LootListID == 3);
    CHECK(Playerbots::PacketParse::IsStartLootRollMethod(parsed.Method));
    CHECK(Playerbots::PacketParse::VERIFIED_BUILD == 68453u);
}

TEST_CASE("Playerbots StartLootRoll NEED_BEFORE_GREED parse round-trip (Layer 3 fixture)", "[playerbots][packet]")
{
    WorldPackets::Loot::StartLootRoll packet;
    packet.LootObj = ObjectGuid::Create<HighGuid::LootObject>(0, 0, 66);
    packet.MapID = 1;
    packet.RollTime = 60000ms;
    packet.ValidRolls = ROLL_FLAG_TYPE_PASS | ROLL_FLAG_TYPE_GREED;
    packet.Method = NEED_BEFORE_GREED;
    packet.DungeonEncounterID = 1234;
    packet.Item.Type = LootItemType::Item;
    packet.Item.UIType = LOOT_SLOT_TYPE_ROLL_ONGOING;
    packet.Item.Loot.ItemID = 2589;
    packet.Item.Quantity = 2;
    packet.Item.LootListID = 1;

    WorldPacket wire = BuildStartLootRollFixture(packet);

    Playerbots::PacketParse::StartLootRollPayload parsed;
    REQUIRE(Playerbots::PacketParse::TryReadStartLootRoll(wire, parsed));

    CHECK(parsed.MapID == 1);
    CHECK(parsed.Method == uint8(NEED_BEFORE_GREED));
    CHECK(parsed.DungeonEncounterID == 1234);
    CHECK(parsed.ValidRolls == uint8(ROLL_FLAG_TYPE_PASS | ROLL_FLAG_TYPE_GREED));
    CHECK(parsed.Item.Loot.ItemID == 2589u);
    CHECK(parsed.Item.Quantity == 2u);
    CHECK(Playerbots::PacketParse::IsStartLootRollMethod(parsed.Method));
    CHECK(Playerbots::PacketParse::VERIFIED_BUILD == 68453u);
}

TEST_CASE("Playerbots StartLootRoll truncated payload fails Layer 1 loudly", "[playerbots][packet]")
{
    WorldPackets::Loot::StartLootRoll packet;
    packet.LootObj = ObjectGuid::Create<HighGuid::LootObject>(0, 0, 2);
    packet.MapID = 0;
    packet.RollTime = 60000ms;
    packet.ValidRolls = ROLL_ALL_TYPE_NO_DISENCHANT;
    packet.Method = GROUP_LOOT;
    packet.Item.Type = LootItemType::Item;
    packet.Item.UIType = LOOT_SLOT_TYPE_ROLL_ONGOING;
    packet.Item.Loot.ItemID = 12345;
    packet.Item.Quantity = 1;
    packet.Item.LootListID = 1;

    WorldPacket wire = BuildStartLootRollFixture(packet);
    REQUIRE(wire.size() > 8);
    wire.resize(wire.size() / 2);

    Playerbots::PacketParse::StartLootRollPayload parsed;
    CHECK_FALSE(Playerbots::PacketParse::TryReadStartLootRoll(wire, parsed));
}
