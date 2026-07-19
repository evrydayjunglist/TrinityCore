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

#include "Bot/Packet/BotCastFailedPacket.h"
#include "Bot/Packet/BotPacketParse.h"
#include "ObjectGuid.h"
#include "Opcodes.h"
#include "SharedDefines.h"
#include "Spell.h"
#include "SpellPackets.h"
#include "WorldPacket.h"

namespace
{
WorldPacket BuildCastFailedFixture(WorldPackets::Spells::CastFailed& packet)
{
    WorldPacket const* written = packet.Write();
    WorldPacket copy(*written);
    copy.SetOpcode(SMSG_CAST_FAILED);
    return copy;
}
}

TEST_CASE("Playerbots CastFailed base fail parse round-trip (Layer 3 fixture)", "[playerbots][packet]")
{
    WorldPackets::Spells::CastFailed packet;
    packet.CastID = ObjectGuid::Create<HighGuid::Cast>(SPELL_CAST_SOURCE_NORMAL, 0, 133, 7);
    packet.SpellID = 133;
    packet.Visual.SpellXSpellVisualID = 42;
    packet.Visual.ScriptVisualID = 0;
    packet.Reason = SPELL_FAILED_NOT_READY;
    packet.FailedArg1 = -1;
    packet.FailedArg2 = -1;
    packet.FailedBy = ObjectGuid::Empty;

    WorldPacket wire = BuildCastFailedFixture(packet);

    Playerbots::PacketParse::CastFailedPayload parsed;
    REQUIRE(Playerbots::PacketParse::TryReadCastFailed(wire, parsed));

    CHECK(parsed.CastID == ObjectGuid::Create<HighGuid::Cast>(SPELL_CAST_SOURCE_NORMAL, 0, 133, 7));
    CHECK(parsed.SpellID == 133);
    CHECK(parsed.Visual.SpellXSpellVisualID == 42);
    CHECK(parsed.Visual.ScriptVisualID == 0);
    CHECK(parsed.Reason == SPELL_FAILED_NOT_READY);
    CHECK(parsed.FailedArg1 == -1);
    CHECK(parsed.FailedArg2 == -1);
    CHECK(parsed.FailedBy.IsEmpty());
    CHECK(Playerbots::PacketParse::VERIFIED_BUILD == 68453u);
}

TEST_CASE("Playerbots CastFailed Arg-filled Reason parse round-trip (Layer 3 fixture)", "[playerbots][packet]")
{
    WorldPackets::Spells::CastFailed packet;
    packet.CastID = ObjectGuid::Create<HighGuid::Cast>(SPELL_CAST_SOURCE_NORMAL, 0, 5143, 9);
    packet.SpellID = 5143; // Arcane Missiles-era sample id (layout only)
    packet.Visual.SpellXSpellVisualID = 100;
    packet.Visual.ScriptVisualID = 2;
    packet.Reason = SPELL_FAILED_REQUIRES_SPELL_FOCUS;
    packet.FailedArg1 = 4; // SpellFocusObject id shape
    packet.FailedArg2 = -1;
    packet.FailedBy = ObjectGuid::Empty;

    WorldPacket wire = BuildCastFailedFixture(packet);

    Playerbots::PacketParse::CastFailedPayload parsed;
    REQUIRE(Playerbots::PacketParse::TryReadCastFailed(wire, parsed));

    CHECK(parsed.SpellID == 5143);
    CHECK(parsed.Visual.SpellXSpellVisualID == 100);
    CHECK(parsed.Visual.ScriptVisualID == 2);
    CHECK(parsed.Reason == SPELL_FAILED_REQUIRES_SPELL_FOCUS);
    CHECK(parsed.FailedArg1 == 4);
    CHECK(parsed.FailedArg2 == -1);
}

TEST_CASE("Playerbots CastFailed truncated payload fails Layer 1 loudly", "[playerbots][packet]")
{
    WorldPackets::Spells::CastFailed packet;
    packet.CastID = ObjectGuid::Create<HighGuid::Cast>(SPELL_CAST_SOURCE_NORMAL, 0, 1, 1);
    packet.SpellID = 1;
    packet.Reason = SPELL_FAILED_DONT_REPORT;

    WorldPacket wire = BuildCastFailedFixture(packet);
    REQUIRE(wire.size() > 4);
    wire.resize(4);

    Playerbots::PacketParse::CastFailedPayload parsed;
    CHECK_FALSE(Playerbots::PacketParse::TryReadCastFailed(wire, parsed));
}
