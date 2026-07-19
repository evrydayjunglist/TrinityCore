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

#include "BotMoveSetRunSpeedPacket.h"
#include "BotPacketParse.h"

namespace Playerbots::PacketParse
{
namespace
{
void ReadMoveSetRunSpeedBody(ByteBuffer& data, MoveSetRunSpeedPayload& out)
{
    // Mirror WorldPackets::Movement::MoveSetSpeed::Write() field order exactly.
    data >> out.MoverGUID;
    data >> out.SequenceIndex;
    data >> out.Speed;
}
}

bool TryReadMoveSetRunSpeed(WorldPacket const& packet, MoveSetRunSpeedPayload& out)
{
    MoveSetRunSpeedPayload parsed;
    Result const result = TryReadFully(packet, "SMSG_MOVE_SET_RUN_SPEED", [&parsed](WorldPacket& copy)
    {
        ReadMoveSetRunSpeedBody(copy, parsed);
    });

    if (result != Result::Ok)
        return false;

    out = parsed;
    return true;
}
}
