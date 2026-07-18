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

#include "Bot/Packet/BotLogXPGainPacket.h"
#include "Bot/Packet/BotPacketParse.h"
#include "CharacterPackets.h"
#include "ObjectGuid.h"
#include "Opcodes.h"
#include "Player.h"
#include "WorldPacket.h"

namespace
{
WorldPacket BuildLogXPGainFixture(WorldPackets::Character::LogXPGain& packet)
{
    WorldPacket const* written = packet.Write();
    WorldPacket copy(*written);
    copy.SetOpcode(SMSG_LOG_XP_GAIN);
    return copy;
}
}

TEST_CASE("Playerbots LogXPGain kill parse round-trip (Layer 3 fixture)", "[playerbots][packet]")
{
    WorldPackets::Character::LogXPGain packet;
    packet.Victim = ObjectGuid::Create<HighGuid::Creature>(1, 100, 1);
    packet.Original = 150;
    packet.Reason = LOG_XP_REASON_KILL;
    packet.Amount = 100;
    packet.GroupBonus = 1.0f;

    WorldPacket wire = BuildLogXPGainFixture(packet);

    Playerbots::PacketParse::LogXPGainPayload parsed;
    REQUIRE(Playerbots::PacketParse::TryReadLogXPGain(wire, parsed));

    CHECK(parsed.Victim == ObjectGuid::Create<HighGuid::Creature>(1, 100, 1));
    CHECK(parsed.Original == 150);
    CHECK(parsed.Reason == LOG_XP_REASON_KILL);
    CHECK(parsed.Amount == 100);
    CHECK(parsed.GroupBonus == 1.0f);
    CHECK(Playerbots::PacketParse::VERIFIED_BUILD == 68453u);
}

TEST_CASE("Playerbots LogXPGain no-kill parse round-trip (Layer 3 fixture)", "[playerbots][packet]")
{
    WorldPackets::Character::LogXPGain packet;
    packet.Victim = ObjectGuid::Empty;
    packet.Original = 50;
    packet.Reason = LOG_XP_REASON_NO_KILL;
    packet.Amount = 50;
    packet.GroupBonus = 0.0f;

    WorldPacket wire = BuildLogXPGainFixture(packet);

    Playerbots::PacketParse::LogXPGainPayload parsed;
    REQUIRE(Playerbots::PacketParse::TryReadLogXPGain(wire, parsed));

    CHECK(parsed.Victim.IsEmpty());
    CHECK(parsed.Original == 50);
    CHECK(parsed.Reason == LOG_XP_REASON_NO_KILL);
    CHECK(parsed.Amount == 50);
    CHECK(parsed.GroupBonus == 0.0f);
}

TEST_CASE("Playerbots LogXPGain truncated payload fails Layer 1 loudly", "[playerbots][packet]")
{
    WorldPackets::Character::LogXPGain packet;
    packet.Victim = ObjectGuid::Empty;
    packet.Original = 10;
    packet.Reason = LOG_XP_REASON_NO_KILL;
    packet.Amount = 10;
    packet.GroupBonus = 0.0f;

    WorldPacket wire = BuildLogXPGainFixture(packet);
    REQUIRE(wire.size() > 4);
    wire.resize(4);

    Playerbots::PacketParse::LogXPGainPayload parsed;
    CHECK_FALSE(Playerbots::PacketParse::TryReadLogXPGain(wire, parsed));
}
