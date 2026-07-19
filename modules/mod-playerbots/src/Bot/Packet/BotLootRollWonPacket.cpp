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

#include "BotLootRollWonPacket.h"
#include "BotLootItemData.h"
#include "BotPacketParse.h"
#include "Loot.h"
#include "PacketOperators.h"

namespace Playerbots::PacketParse
{
namespace
{
void ReadLootRollWonBody(ByteBuffer& data, LootRollWonPayload& out)
{
    // Mirror WorldPackets::Loot::LootRollWon::Write() field order exactly.
    data >> out.LootObj;
    data >> out.Winner;
    data >> out.Roll;
    data >> out.RollType;
    data >> out.DungeonEncounterID;
    ReadLootItemData(data, out.Item);
    data >> WorldPackets::Bits<1>(out.MainSpec);
    data.ResetBitPos();
}
}

bool IsKnownLootRollWonRollType(uint8 rollType)
{
    switch (RollVote(rollType))
    {
        case RollVote::Pass:
        case RollVote::Need:
        case RollVote::Greed:
        case RollVote::Disenchant:
            return true;
        default:
            return false;
    }
}

bool TryReadLootRollWon(WorldPacket const& packet, LootRollWonPayload& out)
{
    LootRollWonPayload parsed;
    Result const result = TryReadFully(packet, "SMSG_LOOT_ROLL_WON", [&parsed](WorldPacket& copy)
    {
        ReadLootRollWonBody(copy, parsed);
    });

    if (result != Result::Ok)
        return false;

    out = std::move(parsed);
    return true;
}
}
