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

#include "BotResurrectRequestPacket.h"
#include "BotPacketParse.h"
#include "PacketOperators.h"

namespace Playerbots::PacketParse
{
namespace
{
void ReadResurrectRequestBody(ByteBuffer& data, ResurrectRequestPayload& out)
{
    // Mirror WorldPackets::Spells::ResurrectRequest::Write() field order exactly.
    data >> out.ResurrectOffererGUID;
    data >> out.ResurrectOffererVirtualRealmAddress;
    data >> out.PetNumber;
    data >> out.SpellID;
    data >> WorldPackets::SizedString::BitsSize<11>(out.Name);
    data >> WorldPackets::Bits<1>(out.UseTimer);
    data >> WorldPackets::Bits<1>(out.Sickness);
    data.ResetBitPos();
    data >> WorldPackets::SizedString::Data(out.Name);
}
}

bool TryReadResurrectRequest(WorldPacket const& packet, ResurrectRequestPayload& out)
{
    ResurrectRequestPayload parsed;
    Result const result = TryReadFully(packet, "SMSG_RESURRECT_REQUEST", [&parsed](WorldPacket& copy)
    {
        ReadResurrectRequestBody(copy, parsed);
    });

    if (result != Result::Ok)
        return false;

    out = std::move(parsed);
    return true;
}
}
