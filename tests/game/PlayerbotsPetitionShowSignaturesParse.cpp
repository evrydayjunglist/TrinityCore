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

#include "tc_catch2.h"

#include "Bot/Packet/BotPacketParse.h"
#include "Bot/Packet/BotPetitionShowSignaturesPacket.h"
#include "ObjectGuid.h"
#include "Opcodes.h"
#include "PetitionPackets.h"
#include "WorldPacket.h"

namespace
{
WorldPacket BuildPetitionShowSignaturesFixture()
{
    WorldPackets::Petition::ServerPetitionShowSignatures packet;
    packet.Item = ObjectGuid::Create<HighGuid::Item>(1001);
    packet.Owner = ObjectGuid::Create<HighGuid::Player>(42);
    packet.OwnerAccountID = ObjectGuid::Create<HighGuid::WowAccount>(7);
    packet.PetitionID = 1001;

    WorldPackets::Petition::ServerPetitionShowSignatures::PetitionSignature signature;
    signature.Signer = ObjectGuid::Create<HighGuid::Player>(99);
    signature.Choice = 0;
    packet.Signatures.push_back(signature);

    WorldPacket const* written = packet.Write();
    WorldPacket copy(*written);
    copy.SetOpcode(SMSG_PETITION_SHOW_SIGNATURES);
    return copy;
}
}

TEST_CASE("Playerbots PetitionShowSignatures payload parse round-trip (Layer 3 fixture)", "[playerbots][packet]")
{
    WorldPacket packet = BuildPetitionShowSignaturesFixture();

    Playerbots::PacketParse::PetitionShowSignaturesPayload parsed;
    REQUIRE(Playerbots::PacketParse::TryReadPetitionShowSignatures(packet, parsed));

    CHECK(parsed.Item == ObjectGuid::Create<HighGuid::Item>(1001));
    CHECK(parsed.Owner == ObjectGuid::Create<HighGuid::Player>(42));
    CHECK(parsed.OwnerAccountID == ObjectGuid::Create<HighGuid::WowAccount>(7));
    CHECK(parsed.PetitionID == 1001);
    REQUIRE(parsed.Signatures.size() == 1);
    CHECK(parsed.Signatures[0].Signer == ObjectGuid::Create<HighGuid::Player>(99));
    CHECK(parsed.Signatures[0].Choice == 0);
    CHECK(Playerbots::PacketParse::VERIFIED_BUILD == 68453u);
}

TEST_CASE("Playerbots PetitionShowSignatures truncated payload fails Layer 1 loudly", "[playerbots][packet]")
{
    WorldPacket packet = BuildPetitionShowSignaturesFixture();
    REQUIRE(packet.size() > 4);
    packet.resize(packet.size() / 2);

    Playerbots::PacketParse::PetitionShowSignaturesPayload parsed;
    CHECK_FALSE(Playerbots::PacketParse::TryReadPetitionShowSignatures(packet, parsed));
}
