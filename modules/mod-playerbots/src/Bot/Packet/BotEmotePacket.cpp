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

#include "BotEmotePacket.h"
#include "BotPacketParse.h"
#include "PacketOperators.h"

namespace Playerbots::PacketParse
{
namespace
{
void ReadEmoteBody(ByteBuffer& data, EmotePayload& out)
{
    // Mirror WorldPackets::Chat::Emote::Write() field order exactly
    // (Guid before EmoteID — not AC WotLK emoteId-then-source).
    data >> out.Guid;
    data >> out.EmoteID;
    data >> WorldPackets::Size<uint32>(out.SpellVisualKitIDs);
    data >> out.SequenceVariation;
    for (int32& kitId : out.SpellVisualKitIDs)
        data >> kitId;
}
}

bool TryReadEmote(WorldPacket const& packet, EmotePayload& out)
{
    EmotePayload parsed;
    Result const result = TryReadFully(packet, "SMSG_EMOTE", [&parsed](WorldPacket& copy)
    {
        ReadEmoteBody(copy, parsed);
    });

    if (result != Result::Ok)
        return false;

    out = std::move(parsed);
    return true;
}
}
