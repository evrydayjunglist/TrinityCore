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

#include "BotLogXPGainPacket.h"
#include "BotPacketParse.h"

namespace Playerbots::PacketParse
{
namespace
{
void ReadLogXPGainBody(ByteBuffer& data, LogXPGainPayload& out)
{
    // Mirror WorldPackets::Character::LogXPGain::Write() field order exactly.
    data >> out.Victim;
    data >> out.Original;
    data >> out.Reason;
    data >> out.Amount;
    data >> out.GroupBonus;
}
}

bool TryReadLogXPGain(WorldPacket const& packet, LogXPGainPayload& out)
{
    LogXPGainPayload parsed;
    Result const result = TryReadFully(packet, "SMSG_LOG_XP_GAIN", [&parsed](WorldPacket& copy)
    {
        ReadLogXPGainBody(copy, parsed);
    });

    if (result != Result::Ok)
        return false;

    out = std::move(parsed);
    return true;
}
}
