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

#include "BotPetitionShowSignaturesPacket.h"
#include "BotPacketParse.h"
#include "PacketOperators.h"

namespace Playerbots::PacketParse
{
namespace
{
void ReadPetitionShowSignaturesBody(ByteBuffer& data, PetitionShowSignaturesPayload& out)
{
    // Mirror WorldPackets::Petition::ServerPetitionShowSignatures::Write() field order exactly.
    data >> out.Item;
    data >> out.Owner;
    data >> out.OwnerAccountID;
    data >> out.PetitionID;
    data >> WorldPackets::Size<uint32>(out.Signatures);

    for (PetitionSignaturePayload& signature : out.Signatures)
    {
        data >> signature.Signer;
        data >> signature.Choice;
    }
}
}

bool TryReadPetitionShowSignatures(WorldPacket const& packet, PetitionShowSignaturesPayload& out)
{
    PetitionShowSignaturesPayload parsed;
    Result const result = TryReadFully(packet, "SMSG_PETITION_SHOW_SIGNATURES", [&parsed](WorldPacket& copy)
    {
        ReadPetitionShowSignaturesBody(copy, parsed);
    });

    if (result != Result::Ok)
        return false;

    out = std::move(parsed);
    return true;
}
}
