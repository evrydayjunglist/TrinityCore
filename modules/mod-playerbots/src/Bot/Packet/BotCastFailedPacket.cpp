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

#include "BotCastFailedPacket.h"
#include "BotPacketParse.h"

namespace Playerbots::PacketParse
{
namespace
{
void ReadCastFailedBody(ByteBuffer& data, CastFailedPayload& out)
{
    // Mirror WorldPackets::Spells::CastFailed::Write() field order exactly
    // (Visual = SpellXSpellVisualID then ScriptVisualID).
    data >> out.CastID;
    data >> out.SpellID;
    data >> out.Visual.SpellXSpellVisualID;
    data >> out.Visual.ScriptVisualID;
    data >> out.Reason;
    data >> out.FailedArg1;
    data >> out.FailedArg2;
    data >> out.FailedBy;
}
}

bool TryReadCastFailed(WorldPacket const& packet, CastFailedPayload& out)
{
    CastFailedPayload parsed;
    Result const result = TryReadFully(packet, "SMSG_CAST_FAILED", [&parsed](WorldPacket& copy)
    {
        ReadCastFailedBody(copy, parsed);
    });

    if (result != Result::Ok)
        return false;

    out = std::move(parsed);
    return true;
}
}
