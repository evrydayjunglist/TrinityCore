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

#include "BotStartLootRollPacket.h"
#include "BotLootItemData.h"
#include "BotPacketParse.h"

namespace Playerbots::PacketParse
{
namespace
{
void ReadStartLootRollBody(ByteBuffer& data, StartLootRollPayload& out)
{
    // Mirror WorldPackets::Loot::StartLootRoll::Write() field order exactly.
    data >> out.LootObj;
    data >> out.MapID;
    // Duration<Milliseconds, uint32> on the wire.
    data >> out.RollTime;
    data >> out.ValidRolls;
    for (LootRollIneligibilityReason& reason : out.LootRollIneligibleReason)
    {
        uint32 raw = 0;
        data >> raw;
        reason = static_cast<LootRollIneligibilityReason>(raw);
    }
    data >> out.Method;
    data >> out.DungeonEncounterID;
    ReadLootItemData(data, out.Item);
}
}

bool IsStartLootRollMethod(uint8 method)
{
    switch (LootMethod(method))
    {
        case GROUP_LOOT:
        case NEED_BEFORE_GREED:
            return true;
        default:
            return false;
    }
}

bool TryReadStartLootRoll(WorldPacket const& packet, StartLootRollPayload& out)
{
    StartLootRollPayload parsed;
    Result const result = TryReadFully(packet, "SMSG_START_LOOT_ROLL", [&parsed](WorldPacket& copy)
    {
        ReadStartLootRollBody(copy, parsed);
    });

    if (result != Result::Ok)
        return false;

    out = std::move(parsed);
    return true;
}
}
