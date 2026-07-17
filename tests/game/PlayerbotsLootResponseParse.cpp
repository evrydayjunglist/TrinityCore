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

#include "Bot/Packet/BotLootResponsePacket.h"
#include "Bot/Packet/BotPacketParse.h"
#include "Loot.h"
#include "LootItemType.h"
#include "LootPackets.h"
#include "ObjectGuid.h"
#include "Opcodes.h"
#include "WorldPacket.h"

namespace
{
WorldPacket BuildLootResponseFixture(WorldPackets::Loot::LootResponse& packet)
{
    WorldPacket const* written = packet.Write();
    WorldPacket copy(*written);
    copy.SetOpcode(SMSG_LOOT_RESPONSE);
    return copy;
}
}

TEST_CASE("Playerbots LootResponse empty acquired window parse round-trip (Layer 3 fixture)", "[playerbots][packet]")
{
    WorldPackets::Loot::LootResponse packet;
    packet.Owner = ObjectGuid::Create<HighGuid::Creature>(0, 100, 1);
    packet.LootObj = ObjectGuid::Create<HighGuid::LootObject>(0, 0, 200);
    packet.FailureReason = LOOT_ERROR_NO_LOOT;
    packet.AcquireReason = 1;
    packet._LootMethod = 0;
    packet.Threshold = 2;
    packet.Coins = 0;
    packet.Acquired = true;
    packet.AELooting = false;
    packet.SuppressError = false;

    WorldPacket wire = BuildLootResponseFixture(packet);

    Playerbots::PacketParse::LootResponsePayload parsed;
    REQUIRE(Playerbots::PacketParse::TryReadLootResponse(wire, parsed));

    CHECK(parsed.Owner == ObjectGuid::Create<HighGuid::Creature>(0, 100, 1));
    CHECK(parsed.LootObj == ObjectGuid::Create<HighGuid::LootObject>(0, 0, 200));
    CHECK(parsed.FailureReason == uint8(LOOT_ERROR_NO_LOOT));
    CHECK(parsed.AcquireReason == 1);
    CHECK(parsed.LootMethod == 0);
    CHECK(parsed.Threshold == 2);
    CHECK(parsed.Coins == 0u);
    CHECK(parsed.Items.empty());
    CHECK(parsed.Currencies.empty());
    CHECK(parsed.Acquired == true);
    CHECK(parsed.AELooting == false);
    CHECK(parsed.SuppressError == false);
    CHECK(Playerbots::PacketParse::IsKnownLootError(parsed.FailureReason));
    CHECK(Playerbots::PacketParse::VERIFIED_BUILD == 68453u);
}

TEST_CASE("Playerbots LootResponse one item + coins parse round-trip (Layer 3 fixture)", "[playerbots][packet]")
{
    WorldPackets::Loot::LootResponse packet;
    packet.Owner = ObjectGuid::Create<HighGuid::Creature>(0, 55, 1);
    packet.LootObj = ObjectGuid::Create<HighGuid::LootObject>(0, 0, 66);
    packet.FailureReason = 0;
    packet.Coins = 1234;
    packet.Acquired = true;

    WorldPackets::Loot::LootItemData item;
    item.Type = LootItemType::Item;
    item.UIType = LOOT_SLOT_TYPE_ALLOW_LOOT;
    item.CanTradeToTapList = false;
    item.Loot.ItemID = 6948;
    item.Quantity = 2;
    item.LootItemType = 0;
    item.LootListID = 3;
    packet.Items.push_back(item);

    WorldPacket wire = BuildLootResponseFixture(packet);

    Playerbots::PacketParse::LootResponsePayload parsed;
    REQUIRE(Playerbots::PacketParse::TryReadLootResponse(wire, parsed));

    CHECK(parsed.Coins == 1234u);
    REQUIRE(parsed.Items.size() == 1);
    CHECK(parsed.Items[0].Type == LootItemType::Item);
    CHECK(parsed.Items[0].UIType == uint8(LOOT_SLOT_TYPE_ALLOW_LOOT));
    CHECK(parsed.Items[0].CanTradeToTapList == false);
    CHECK(parsed.Items[0].Loot.ItemID == 6948);
    CHECK(parsed.Items[0].Quantity == 2u);
    CHECK(parsed.Items[0].LootListID == 3);
    CHECK(parsed.Acquired == true);
    CHECK(Playerbots::PacketParse::VERIFIED_BUILD == 68453u);
}

TEST_CASE("Playerbots LootResponse truncated payload fails Layer 1 loudly", "[playerbots][packet]")
{
    WorldPackets::Loot::LootResponse packet;
    packet.Owner = ObjectGuid::Create<HighGuid::Creature>(0, 1, 1);
    packet.LootObj = ObjectGuid::Create<HighGuid::LootObject>(0, 0, 2);
    packet.Coins = 50;
    packet.Acquired = true;

    WorldPackets::Loot::LootItemData item;
    item.Type = LootItemType::Item;
    item.UIType = LOOT_SLOT_TYPE_OWNER;
    item.Loot.ItemID = 12345;
    item.Quantity = 1;
    item.LootListID = 1;
    packet.Items.push_back(item);

    WorldPacket wire = BuildLootResponseFixture(packet);
    REQUIRE(wire.size() > 8);
    wire.resize(wire.size() / 2);

    Playerbots::PacketParse::LootResponsePayload parsed;
    CHECK_FALSE(Playerbots::PacketParse::TryReadLootResponse(wire, parsed));
}
