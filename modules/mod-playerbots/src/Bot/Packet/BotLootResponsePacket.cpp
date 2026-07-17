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

#include "BotLootResponsePacket.h"
#include "BotPacketParse.h"
#include "ItemPacketsCommon.h"
#include "Loot.h"
#include "LootItemType.h"
#include "PacketOperators.h"

namespace Playerbots::PacketParse
{
namespace
{
void ReadLootItemData(ByteBuffer& data, WorldPackets::Loot::LootItemData& out)
{
    // Mirror WorldPackets::Loot::operator<<(LootItemData) field order exactly.
    uint8 typeRaw = 0;
    data >> WorldPackets::Bits<2>(typeRaw);
    data >> WorldPackets::Bits<3>(out.UIType);
    data >> WorldPackets::Bits<1>(out.CanTradeToTapList);
    data.ResetBitPos();

    out.Type = static_cast<::LootItemType>(typeRaw);
    data >> out.Loot;
    data >> out.Quantity;
    data >> out.LootItemType;
    data >> out.LootListID;
}

void ReadLootCurrency(ByteBuffer& data, WorldPackets::Loot::LootCurrency& out)
{
    // Mirror WorldPackets::Loot::operator<<(LootCurrency) field order exactly.
    data >> out.CurrencyID;
    data >> out.Quantity;
    data >> out.LootListID;
    data >> WorldPackets::Bits<3>(out.UIType);
    data.ResetBitPos();
}

void ReadLootResponseBody(ByteBuffer& data, LootResponsePayload& out)
{
    // Mirror WorldPackets::Loot::LootResponse::Write() field order exactly.
    data >> out.Owner;
    data >> out.LootObj;
    data >> out.FailureReason;
    data >> out.AcquireReason;
    data >> out.LootMethod;
    data >> out.Threshold;
    data >> out.Coins;
    data >> WorldPackets::Size<uint32>(out.Items);
    data >> WorldPackets::Size<uint32>(out.Currencies);
    data >> WorldPackets::Bits<1>(out.Acquired);
    data >> WorldPackets::Bits<1>(out.AELooting);
    data >> WorldPackets::Bits<1>(out.SuppressError);
    data.ResetBitPos();

    for (WorldPackets::Loot::LootItemData& item : out.Items)
        ReadLootItemData(data, item);

    for (WorldPackets::Loot::LootCurrency& currency : out.Currencies)
        ReadLootCurrency(data, currency);
}
}

bool IsKnownLootError(uint8 failureReason)
{
    switch (LootError(failureReason))
    {
        case LOOT_ERROR_DIDNT_KILL:
        case LOOT_ERROR_TOO_FAR:
        case LOOT_ERROR_BAD_FACING:
        case LOOT_ERROR_LOCKED:
        case LOOT_ERROR_NOTSTANDING:
        case LOOT_ERROR_STUNNED:
        case LOOT_ERROR_PLAYER_NOT_FOUND:
        case LOOT_ERROR_PLAY_TIME_EXCEEDED:
        case LOOT_ERROR_MASTER_INV_FULL:
        case LOOT_ERROR_MASTER_UNIQUE_ITEM:
        case LOOT_ERROR_MASTER_OTHER:
        case LOOT_ERROR_ALREADY_PICKPOCKETED:
        case LOOT_ERROR_NOT_WHILE_SHAPESHIFTED:
        case LOOT_ERROR_NO_LOOT:
            return true;
        default:
            return false;
    }
}

bool TryReadLootResponse(WorldPacket const& packet, LootResponsePayload& out)
{
    LootResponsePayload parsed;
    Result const result = TryReadFully(packet, "SMSG_LOOT_RESPONSE", [&parsed](WorldPacket& copy)
    {
        ReadLootResponseBody(copy, parsed);
    });

    if (result != Result::Ok)
        return false;

    out = std::move(parsed);
    return true;
}
}
