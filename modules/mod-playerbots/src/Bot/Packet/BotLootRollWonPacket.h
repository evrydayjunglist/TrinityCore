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

#ifndef TRINITY_BOT_LOOT_ROLL_WON_PACKET_H
#define TRINITY_BOT_LOOT_ROLL_WON_PACKET_H

#include "LootPackets.h"
#include "WorldPacket.h"

namespace Playerbots::PacketParse
{
// Fields mirrored from WorldPackets::Loot::LootRollWon::Write() (+ nested LootItemData).
struct LootRollWonPayload
{
    ObjectGuid LootObj;
    ObjectGuid Winner;
    int32 Roll = 0;
    uint8 RollType = 0;
    int32 DungeonEncounterID = 0;
    WorldPackets::Loot::LootItemData Item;
    bool MainSpec = false;
};

// Pass / Need / Greed / Disenchant (RollVote 0–3) — winner-ish set from SendLootRollWon.
bool IsKnownLootRollWonRollType(uint8 rollType);

// Returns true on Layer-1 success (full consume, no exception). Does not perform Layer 2.
bool TryReadLootRollWon(WorldPacket const& packet, LootRollWonPayload& out);
}

#endif
