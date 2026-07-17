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

#ifndef TRINITY_BOT_TRADE_UPDATED_PACKET_H
#define TRINITY_BOT_TRADE_UPDATED_PACKET_H

#include "TradePackets.h"
#include "WorldPacket.h"
#include <vector>

namespace Playerbots::PacketParse
{
// Fields mirrored from WorldPackets::Trade::TradeUpdated::Write() (+ nested TradeItem).
struct TradeUpdatedPayload
{
    uint8 WhichPlayer = 0;
    uint32 ID = 0;
    uint32 ClientStateIndex = 0;
    uint32 CurrentStateIndex = 0;
    uint64 Gold = 0;
    int32 CurrencyType = 0;
    int32 CurrencyQuantity = 0;
    int32 ProposedEnchantment = 0;
    std::vector<WorldPackets::Trade::TradeItem> Items;
};

// Returns true on Layer-1 success (full consume, no exception). Does not perform Layer 2.
bool TryReadTradeUpdated(WorldPacket const& packet, TradeUpdatedPayload& out);
}

#endif
