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

#include "Bot/Packet/BotLevelUpInfoPacket.h"
#include "Bot/Packet/BotPacketParse.h"
#include "MiscPackets.h"
#include "Opcodes.h"
#include "SharedDefines.h"
#include "WorldPacket.h"

namespace
{
WorldPacket BuildLevelUpInfoFixture(int32 level)
{
    WorldPackets::Misc::LevelUpInfo packet;
    packet.Level = level;
    packet.HealthDelta = 0;
    packet.PowerDelta.fill(0);
    packet.StatDelta.fill(0);
    packet.NumNewTalents = 0;
    packet.NumNewPvpTalentSlots = 0;

    WorldPacket const* written = packet.Write();
    WorldPacket copy(*written);
    copy.SetOpcode(SMSG_LEVEL_UP_INFO);
    return copy;
}
}

TEST_CASE("Playerbots LevelUpInfo parse round-trip (Layer 3 fixture)", "[playerbots][packet]")
{
    WorldPacket packet = BuildLevelUpInfoFixture(42);

    Playerbots::PacketParse::LevelUpInfoPayload parsed;
    REQUIRE(Playerbots::PacketParse::TryReadLevelUpInfo(packet, parsed));

    CHECK(parsed.Level == 42);
    CHECK(parsed.HealthDelta == 0);
    CHECK(parsed.PowerDelta.size() == MAX_POWERS_PER_CLASS);
    CHECK(parsed.StatDelta.size() == MAX_STATS);
    for (int32 power : parsed.PowerDelta)
        CHECK(power == 0);
    for (int32 stat : parsed.StatDelta)
        CHECK(stat == 0);
    CHECK(parsed.NumNewTalents == 0);
    CHECK(parsed.NumNewPvpTalentSlots == 0);
    CHECK(Playerbots::PacketParse::VERIFIED_BUILD == 68453u);
}

TEST_CASE("Playerbots LevelUpInfo truncated payload fails Layer 1 loudly", "[playerbots][packet]")
{
    WorldPacket packet = BuildLevelUpInfoFixture(10);
    REQUIRE(packet.size() > 4);
    packet.resize(4);

    Playerbots::PacketParse::LevelUpInfoPayload parsed;
    CHECK_FALSE(Playerbots::PacketParse::TryReadLevelUpInfo(packet, parsed));
}
