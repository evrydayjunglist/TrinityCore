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

#include "BotTradeStatusPacket.h"
#include "BotPacketParse.h"
#include "PacketOperators.h"

namespace Playerbots::PacketParse
{
namespace
{
void ReadTradeStatusBody(ByteBuffer& data, TradeStatusPayload& out)
{
    // Mirror WorldPackets::Trade::TradeStatus::Write() field order exactly.
    data >> WorldPackets::Bits<1>(out.PartnerIsSameBnetAccount);
    data >> WorldPackets::Bits<5>(out.Status);

    switch (out.Status)
    {
        case TRADE_STATUS_FAILED:
            data >> WorldPackets::Bits<1>(out.FailureForYou);
            data.ResetBitPos();
            data >> out.BagResult;
            data >> out.ItemID;
            break;
        case TRADE_STATUS_INITIATED:
            data.ResetBitPos();
            data >> out.ID;
            break;
        case TRADE_STATUS_PROPOSED:
            data.ResetBitPos();
            data >> out.Partner;
            data >> out.PartnerAccount;
            break;
        case TRADE_STATUS_WRONG_REALM:
        case TRADE_STATUS_NOT_ON_TAPLIST:
            data.ResetBitPos();
            data >> out.TradeSlot;
            break;
        case TRADE_STATUS_NOT_ENOUGH_CURRENCY:
        case TRADE_STATUS_CURRENCY_NOT_TRADABLE:
            data.ResetBitPos();
            data >> out.CurrencyType;
            data >> out.CurrencyQuantity;
            break;
        default:
            data.ResetBitPos();
            break;
    }
}
}

bool IsKnownTradeStatus(::TradeStatus status)
{
    switch (status)
    {
        case TRADE_STATUS_PLAYER_BUSY:
        case TRADE_STATUS_PROPOSED:
        case TRADE_STATUS_INITIATED:
        case TRADE_STATUS_CANCELLED:
        case TRADE_STATUS_ACCEPTED:
        case TRADE_STATUS_ALREADY_TRADING:
        case TRADE_STATUS_NO_TARGET:
        case TRADE_STATUS_UNACCEPTED:
        case TRADE_STATUS_COMPLETE:
        case TRADE_STATUS_STATE_CHANGED:
        case TRADE_STATUS_TOO_FAR_AWAY:
        case TRADE_STATUS_WRONG_FACTION:
        case TRADE_STATUS_FAILED:
        case TRADE_STATUS_PETITION:
        case TRADE_STATUS_PLAYER_IGNORED:
        case TRADE_STATUS_STUNNED:
        case TRADE_STATUS_TARGET_STUNNED:
        case TRADE_STATUS_DEAD:
        case TRADE_STATUS_TARGET_DEAD:
        case TRADE_STATUS_LOGGING_OUT:
        case TRADE_STATUS_TARGET_LOGGING_OUT:
        case TRADE_STATUS_RESTRICTED_ACCOUNT:
        case TRADE_STATUS_WRONG_REALM:
        case TRADE_STATUS_NOT_ON_TAPLIST:
        case TRADE_STATUS_CURRENCY_NOT_TRADABLE:
        case TRADE_STATUS_NOT_ENOUGH_CURRENCY:
            return true;
        default:
            return false;
    }
}

bool TryReadTradeStatus(WorldPacket const& packet, TradeStatusPayload& out)
{
    TradeStatusPayload parsed;
    Result const result = TryReadFully(packet, "SMSG_TRADE_STATUS", [&parsed](WorldPacket& copy)
    {
        ReadTradeStatusBody(copy, parsed);
    });

    if (result != Result::Ok)
        return false;

    out = parsed;
    return true;
}
}
