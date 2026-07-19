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

#include "BotTradeUpdatedPacket.h"
#include "BotPacketParse.h"
#include "ItemPacketsCommon.h"
#include "PacketOperators.h"

namespace Playerbots::PacketParse
{
namespace
{
void ReadUnwrappedTradeItem(ByteBuffer& data, WorldPackets::Trade::UnwrappedTradeItem& out)
{
    // Mirror WorldPackets::Trade::operator<<(UnwrappedTradeItem) field order exactly.
    data >> out.EnchantID;
    data >> out.OnUseEnchantmentID;
    data >> out.Creator;
    data >> out.Charges;
    data >> out.MaxDurability;
    data >> out.Durability;
    data >> WorldPackets::BitsSize<2>(out.Gems);
    data >> WorldPackets::Bits<1>(out.Lock);
    data.ResetBitPos();

    for (WorldPackets::Item::ItemGemData& gem : out.Gems)
        data >> gem;
}

void ReadTradeItem(ByteBuffer& data, WorldPackets::Trade::TradeItem& out)
{
    // Mirror WorldPackets::Trade::operator<<(TradeItem) field order exactly.
    data >> out.Slot;
    uint32 stackCount = 0;
    data >> stackCount;
    out.StackCount = int32(stackCount);
    data >> out.GiftCreator;
    data >> out.Item;
    data >> WorldPackets::OptionalInit(out.Unwrapped);
    data.ResetBitPos();
    if (out.Unwrapped)
        ReadUnwrappedTradeItem(data, *out.Unwrapped);
}

void ReadTradeUpdatedBody(ByteBuffer& data, TradeUpdatedPayload& out)
{
    // Mirror WorldPackets::Trade::TradeUpdated::Write() field order exactly.
    data >> out.WhichPlayer;
    data >> out.ID;
    data >> out.ClientStateIndex;
    data >> out.CurrentStateIndex;
    data >> out.Gold;
    data >> out.CurrencyType;
    data >> out.CurrencyQuantity;
    data >> out.ProposedEnchantment;
    data >> WorldPackets::Size<uint32>(out.Items);

    for (WorldPackets::Trade::TradeItem& item : out.Items)
        ReadTradeItem(data, item);
}
}

bool TryReadTradeUpdated(WorldPacket const& packet, TradeUpdatedPayload& out)
{
    TradeUpdatedPayload parsed;
    Result const result = TryReadFully(packet, "SMSG_TRADE_UPDATED", [&parsed](WorldPacket& copy)
    {
        ReadTradeUpdatedBody(copy, parsed);
    });

    if (result != Result::Ok)
        return false;

    out = std::move(parsed);
    return true;
}
}
