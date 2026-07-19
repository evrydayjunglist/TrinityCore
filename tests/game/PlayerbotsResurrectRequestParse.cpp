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
#include "Bot/Packet/BotResurrectRequestPacket.h"
#include "ObjectGuid.h"
#include "Opcodes.h"
#include "SpellPackets.h"
#include "WorldPacket.h"

namespace
{
WorldPacket BuildResurrectRequestFixture()
{
    WorldPackets::Spells::ResurrectRequest request;
    request.ResurrectOffererGUID = ObjectGuid::Create<HighGuid::Player>(42);
    request.ResurrectOffererVirtualRealmAddress = 0x01000001;
    request.PetNumber = 0;
    request.SpellID = 2006; // Resurrection (classic priest) — fixture id only
    request.UseTimer = true;
    request.Sickness = false;
    request.Name = "Offerer";

    WorldPacket const* written = request.Write();
    WorldPacket packet(*written);
    packet.SetOpcode(SMSG_RESURRECT_REQUEST);
    return packet;
}
}

TEST_CASE("Playerbots ResurrectRequest payload parse round-trip (Layer 3 fixture)", "[playerbots][packet]")
{
    WorldPacket packet = BuildResurrectRequestFixture();

    Playerbots::PacketParse::ResurrectRequestPayload parsed;
    REQUIRE(Playerbots::PacketParse::TryReadResurrectRequest(packet, parsed));

    CHECK(parsed.ResurrectOffererGUID == ObjectGuid::Create<HighGuid::Player>(42));
    CHECK(parsed.ResurrectOffererVirtualRealmAddress == 0x01000001u);
    CHECK(parsed.PetNumber == 0u);
    CHECK(parsed.SpellID == 2006);
    CHECK(parsed.UseTimer == true);
    CHECK(parsed.Sickness == false);
    CHECK(parsed.Name == "Offerer");
    CHECK(Playerbots::PacketParse::VERIFIED_BUILD == 68453u);
}

TEST_CASE("Playerbots ResurrectRequest truncated payload fails Layer 1 loudly", "[playerbots][packet]")
{
    WorldPacket packet = BuildResurrectRequestFixture();
    REQUIRE(packet.size() > 4);
    packet.resize(packet.size() / 2);

    Playerbots::PacketParse::ResurrectRequestPayload parsed;
    CHECK_FALSE(Playerbots::PacketParse::TryReadResurrectRequest(packet, parsed));
}
