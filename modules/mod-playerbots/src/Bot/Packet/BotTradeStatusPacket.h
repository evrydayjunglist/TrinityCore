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

#ifndef TRINITY_BOT_TRADE_STATUS_PACKET_H
#define TRINITY_BOT_TRADE_STATUS_PACKET_H

#include "ObjectGuid.h"
#include "SharedDefines.h"
#include "WorldPacket.h"

namespace Playerbots::PacketParse
{
// Fields mirrored from WorldPackets::Trade::TradeStatus::Write() (Status-dependent trailers).
struct TradeStatusPayload
{
    bool PartnerIsSameBnetAccount = false;
    ::TradeStatus Status = TRADE_STATUS_INITIATED;
    uint8 TradeSlot = 0;
    ObjectGuid PartnerAccount;
    ObjectGuid Partner;
    int32 CurrencyType = 0;
    int32 CurrencyQuantity = 0;
    bool FailureForYou = false;
    int32 BagResult = 0;
    int32 ItemID = 0;
    uint32 ID = 0;
};

// Returns true on Layer-1 success (full consume, no exception). Does not perform Layer 2.
bool TryReadTradeStatus(WorldPacket const& packet, TradeStatusPayload& out);

bool IsKnownTradeStatus(::TradeStatus status);
}

#endif
