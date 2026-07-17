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

#ifndef TRINITY_BOT_LOOT_RESPONSE_PACKET_H
#define TRINITY_BOT_LOOT_RESPONSE_PACKET_H

#include "LootPackets.h"
#include "WorldPacket.h"

namespace Playerbots::PacketParse
{
// Fields mirrored from WorldPackets::Loot::LootResponse::Write() (+ nested item/currency).
struct LootResponsePayload
{
    ObjectGuid Owner;
    ObjectGuid LootObj;
    uint8 FailureReason = 0;
    uint8 AcquireReason = 0;
    uint8 LootMethod = 0;
    uint8 Threshold = 0;
    uint32 Coins = 0;
    std::vector<WorldPackets::Loot::LootItemData> Items;
    std::vector<WorldPackets::Loot::LootCurrency> Currencies;
    bool Acquired = false;
    bool AELooting = false;
    bool SuppressError = false;
};

// Known LootError values (gaps in the enum are still "known" when listed).
bool IsKnownLootError(uint8 failureReason);

// Returns true on Layer-1 success (full consume, no exception). Does not perform Layer 2.
bool TryReadLootResponse(WorldPacket const& packet, LootResponsePayload& out);
}

#endif
