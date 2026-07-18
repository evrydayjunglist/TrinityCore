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

#include "Bot/Packet/BotGroupDestroyedPacket.h"
#include "Bot/Packet/BotPacketParse.h"
#include "Opcodes.h"
#include "PartyPackets.h"
#include "WorldPacket.h"

namespace
{
WorldPacket BuildGroupDestroyedFixture()
{
    WorldPackets::Party::GroupDestroyed packet;
    WorldPacket const* written = packet.Write();
    WorldPacket copy(*written);
    copy.SetOpcode(SMSG_GROUP_DESTROYED);
    return copy;
}
}

TEST_CASE("Playerbots GroupDestroyed empty payload parse round-trip (Layer 3 fixture)", "[playerbots][packet]")
{
    WorldPacket packet = BuildGroupDestroyedFixture();

    REQUIRE(packet.empty());
    REQUIRE(Playerbots::PacketParse::TryReadGroupDestroyed(packet));
    CHECK(Playerbots::PacketParse::VERIFIED_BUILD == 68453u);
}

TEST_CASE("Playerbots GroupDestroyed non-empty payload fails Layer 1 loudly", "[playerbots][packet]")
{
    WorldPacket packet = BuildGroupDestroyedFixture();
    packet << uint8(0xAB);

    CHECK_FALSE(Playerbots::PacketParse::TryReadGroupDestroyed(packet));
}
