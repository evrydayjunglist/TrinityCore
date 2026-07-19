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

#include "BotLevelUpInfoPacket.h"
#include "BotPacketParse.h"

namespace Playerbots::PacketParse
{
namespace
{
void ReadLevelUpInfoBody(ByteBuffer& data, LevelUpInfoPayload& out)
{
    // Mirror WorldPackets::Misc::LevelUpInfo::Write() field order exactly.
    data >> out.Level;
    data >> out.HealthDelta;

    for (int32& power : out.PowerDelta)
        data >> power;

    for (int32& stat : out.StatDelta)
        data >> stat;

    data >> out.NumNewTalents;
    data >> out.NumNewPvpTalentSlots;
}
}

bool TryReadLevelUpInfo(WorldPacket const& packet, LevelUpInfoPayload& out)
{
    LevelUpInfoPayload parsed;
    Result const result = TryReadFully(packet, "SMSG_LEVEL_UP_INFO", [&parsed](WorldPacket& copy)
    {
        ReadLevelUpInfoBody(copy, parsed);
    });

    if (result != Result::Ok)
        return false;

    out = std::move(parsed);
    return true;
}
}
