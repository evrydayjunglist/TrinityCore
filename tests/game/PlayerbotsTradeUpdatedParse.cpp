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
#include "Bot/Packet/BotTradeUpdatedPacket.h"
#include "ObjectGuid.h"
#include "Opcodes.h"
#include "TradePackets.h"
#include "WorldPacket.h"

namespace
{
WorldPacket BuildTradeUpdatedFixture(WorldPackets::Trade::TradeUpdated& packet)
{
    WorldPacket const* written = packet.Write();
    WorldPacket copy(*written);
    copy.SetOpcode(SMSG_TRADE_UPDATED);
    return copy;
}
}

TEST_CASE("Playerbots TradeUpdated empty-items payload parse round-trip (Layer 3 fixture)", "[playerbots][packet]")
{
    WorldPackets::Trade::TradeUpdated packet;
    packet.WhichPlayer = 1;
    packet.ID = 0;
    packet.ClientStateIndex = 2;
    packet.CurrentStateIndex = 3;
    packet.Gold = 1500;
    packet.CurrencyType = 0;
    packet.CurrencyQuantity = 0;
    packet.ProposedEnchantment = 0;

    WorldPacket wire = BuildTradeUpdatedFixture(packet);

    Playerbots::PacketParse::TradeUpdatedPayload parsed;
    REQUIRE(Playerbots::PacketParse::TryReadTradeUpdated(wire, parsed));

    CHECK(parsed.WhichPlayer == 1);
    CHECK(parsed.ID == 0u);
    CHECK(parsed.ClientStateIndex == 2u);
    CHECK(parsed.CurrentStateIndex == 3u);
    CHECK(parsed.Gold == 1500u);
    CHECK(parsed.CurrencyType == 0);
    CHECK(parsed.CurrencyQuantity == 0);
    CHECK(parsed.ProposedEnchantment == 0);
    CHECK(parsed.Items.empty());
    CHECK(Playerbots::PacketParse::VERIFIED_BUILD == 68453u);
}

TEST_CASE("Playerbots TradeUpdated one simple item payload parse round-trip (Layer 3 fixture)", "[playerbots][packet]")
{
    WorldPackets::Trade::TradeUpdated packet;
    packet.WhichPlayer = 0;
    packet.ClientStateIndex = 1;
    packet.CurrentStateIndex = 4;
    packet.Gold = 0;

    WorldPackets::Trade::TradeItem item;
    item.Slot = 0;
    item.StackCount = 5;
    item.GiftCreator = ObjectGuid::Create<HighGuid::Player>(11);
    item.Item.ItemID = 6948; // Hearthstone-shaped simple ItemID-only instance
    item.Unwrapped.emplace();
    item.Unwrapped->EnchantID = 0;
    item.Unwrapped->OnUseEnchantmentID = 0;
    item.Unwrapped->Creator = ObjectGuid::Create<HighGuid::Player>(22);
    item.Unwrapped->Charges = 0;
    item.Unwrapped->MaxDurability = 0;
    item.Unwrapped->Durability = 0;
    item.Unwrapped->Lock = false;
    packet.Items.push_back(item);

    WorldPacket wire = BuildTradeUpdatedFixture(packet);

    Playerbots::PacketParse::TradeUpdatedPayload parsed;
    REQUIRE(Playerbots::PacketParse::TryReadTradeUpdated(wire, parsed));

    REQUIRE(parsed.Items.size() == 1);
    CHECK(parsed.WhichPlayer == 0);
    CHECK(parsed.CurrentStateIndex == 4u);
    CHECK(parsed.Items[0].Slot == 0);
    CHECK(parsed.Items[0].StackCount == 5);
    CHECK(parsed.Items[0].GiftCreator == ObjectGuid::Create<HighGuid::Player>(11));
    CHECK(parsed.Items[0].Item.ItemID == 6948u);
    REQUIRE(parsed.Items[0].Unwrapped.has_value());
    CHECK(parsed.Items[0].Unwrapped->Creator == ObjectGuid::Create<HighGuid::Player>(22));
    CHECK(parsed.Items[0].Unwrapped->Lock == false);
    CHECK(parsed.Items[0].Unwrapped->Gems.empty());
    CHECK(Playerbots::PacketParse::VERIFIED_BUILD == 68453u);
}

TEST_CASE("Playerbots TradeUpdated truncated payload fails Layer 1 loudly", "[playerbots][packet]")
{
    WorldPackets::Trade::TradeUpdated packet;
    packet.WhichPlayer = 1;
    packet.Gold = 99;
    packet.CurrentStateIndex = 2;

    WorldPackets::Trade::TradeItem item;
    item.Slot = 6;
    item.StackCount = 1;
    item.Item.ItemID = 12345;
    item.Unwrapped.emplace();
    item.Unwrapped->Lock = true;
    packet.Items.push_back(item);

    WorldPacket wire = BuildTradeUpdatedFixture(packet);
    REQUIRE(wire.size() > 8);
    wire.resize(wire.size() / 2);

    Playerbots::PacketParse::TradeUpdatedPayload parsed;
    CHECK_FALSE(Playerbots::PacketParse::TryReadTradeUpdated(wire, parsed));
}
