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

#include "Bot/Packet/BotItemPushResultPacket.h"
#include "Bot/Packet/BotPacketParse.h"
#include "ItemPackets.h"
#include "ObjectGuid.h"
#include "Opcodes.h"
#include "WorldPacket.h"

namespace
{
WorldPacket BuildItemPushResultFixture(WorldPackets::Item::ItemPushResult& packet)
{
    WorldPacket const* written = packet.Write();
    WorldPacket copy(*written);
    copy.SetOpcode(SMSG_ITEM_PUSH_RESULT);
    return copy;
}
}

TEST_CASE("Playerbots ItemPushResult simple self push parse round-trip (Layer 3 fixture)", "[playerbots][packet]")
{
    WorldPackets::Item::ItemPushResult packet;
    packet.PlayerGUID = ObjectGuid::Create<HighGuid::Player>(42);
    packet.Slot = 1;
    packet.SlotInBag = 5;
    packet.ProxyItemID = 0;
    packet.Quantity = 1;
    packet.QuantityInInventory = 3;
    packet.QuantityInQuestLog = 0;
    packet.EncounterID = 0;
    packet.BattlePetSpeciesID = 0;
    packet.BattlePetBreedID = 0;
    packet.BattlePetBreedQuality = 0;
    packet.BattlePetLevel = 0;
    packet.ItemGUID = ObjectGuid::Create<HighGuid::Item>(99);
    packet.Item.ItemID = 6948;
    packet.Pushed = true;
    packet.Created = false;
    packet.FakeQuestItem = false;
    packet.ChatNotifyType = WorldPackets::Item::ItemPushResult::DISPLAY_TYPE_NORMAL;
    packet.IsBonusRoll = false;
    packet.IsPersonalLoot = false;
    // Prefer empty Toasts / no CraftingData / no FirstCraft (typical SendNewItem).

    WorldPacket wire = BuildItemPushResultFixture(packet);

    Playerbots::PacketParse::ItemPushResultPayload parsed;
    REQUIRE(Playerbots::PacketParse::TryReadItemPushResult(wire, parsed));

    CHECK(parsed.PlayerGUID == ObjectGuid::Create<HighGuid::Player>(42));
    CHECK(parsed.Slot == 1);
    CHECK(parsed.SlotInBag == 5);
    CHECK(parsed.ProxyItemID == 0);
    CHECK(parsed.Quantity == 1);
    CHECK(parsed.QuantityInInventory == 3);
    CHECK(parsed.QuantityInQuestLog == 0);
    CHECK(parsed.ItemGUID == ObjectGuid::Create<HighGuid::Item>(99));
    CHECK(parsed.Item.ItemID == 6948u);
    CHECK(parsed.Toasts.empty());
    CHECK_FALSE(parsed.CraftingData.has_value());
    CHECK_FALSE(parsed.FirstCraftOperationID.has_value());
    CHECK(parsed.Pushed == true);
    CHECK(parsed.Created == false);
    CHECK(parsed.ChatNotifyType == WorldPackets::Item::ItemPushResult::DISPLAY_TYPE_NORMAL);
    CHECK(Playerbots::PacketParse::VERIFIED_BUILD == 68453u);
}

TEST_CASE("Playerbots ItemPushResult truncated payload fails Layer 1 loudly", "[playerbots][packet]")
{
    WorldPackets::Item::ItemPushResult packet;
    packet.PlayerGUID = ObjectGuid::Create<HighGuid::Player>(7);
    packet.Quantity = 2;
    packet.QuantityInInventory = 2;
    packet.ItemGUID = ObjectGuid::Create<HighGuid::Item>(11);
    packet.Item.ItemID = 12345;
    packet.Pushed = true;
    packet.ChatNotifyType = WorldPackets::Item::ItemPushResult::DISPLAY_TYPE_NORMAL;

    WorldPacket wire = BuildItemPushResultFixture(packet);
    REQUIRE(wire.size() > 8);
    wire.resize(wire.size() / 2);

    Playerbots::PacketParse::ItemPushResultPayload parsed;
    CHECK_FALSE(Playerbots::PacketParse::TryReadItemPushResult(wire, parsed));
}
