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

#ifndef TRINITY_BOT_START_LOOT_ROLL_PACKET_H
#define TRINITY_BOT_START_LOOT_ROLL_PACKET_H

#include "Loot.h"
#include "LootPackets.h"
#include "WorldPacket.h"
#include <array>

namespace Playerbots::PacketParse
{
// Fields mirrored from WorldPackets::Loot::StartLootRoll::Write() (+ nested LootItemData).
struct StartLootRollPayload
{
    ObjectGuid LootObj;
    int32 MapID = 0;
    uint32 RollTime = 0; // milliseconds (Duration<Milliseconds, uint32>)
    uint8 ValidRolls = 0;
    std::array<LootRollIneligibilityReason, 5> LootRollIneligibleReason = { };
    uint8 Method = 0;
    int32 DungeonEncounterID = 0;
    WorldPackets::Loot::LootItemData Item;
};

// GROUP_LOOT / NEED_BEFORE_GREED — only methods that emit SMSG_START_LOOT_ROLL.
bool IsStartLootRollMethod(uint8 method);

// Returns true on Layer-1 success (full consume, no exception). Does not perform Layer 2.
bool TryReadStartLootRoll(WorldPacket const& packet, StartLootRollPayload& out);
}

#endif
