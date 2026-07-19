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

#include "Bot/Packet/BotEmotePacket.h"
#include "Bot/Packet/BotPacketParse.h"
#include "ChatPackets.h"
#include "ObjectGuid.h"
#include "Opcodes.h"
#include "WorldPacket.h"

namespace
{
WorldPacket BuildEmoteFixture(WorldPackets::Chat::Emote& packet)
{
    WorldPacket const* written = packet.Write();
    WorldPacket copy(*written);
    copy.SetOpcode(SMSG_EMOTE);
    return copy;
}
}

TEST_CASE("Playerbots Emote empty-kits parse round-trip (Layer 3 fixture)", "[playerbots][packet]")
{
    WorldPackets::Chat::Emote packet;
    packet.Guid = ObjectGuid::Create<HighGuid::Player>(42);
    packet.EmoteID = 1; // EMOTE_ONESHOT_TALK shape (layout only)
    packet.SequenceVariation = 0;

    WorldPacket wire = BuildEmoteFixture(packet);

    Playerbots::PacketParse::EmotePayload parsed;
    REQUIRE(Playerbots::PacketParse::TryReadEmote(wire, parsed));

    CHECK(parsed.Guid == ObjectGuid::Create<HighGuid::Player>(42));
    CHECK(parsed.EmoteID == 1u);
    CHECK(parsed.SpellVisualKitIDs.empty());
    CHECK(parsed.SequenceVariation == 0);
    CHECK(Playerbots::PacketParse::VERIFIED_BUILD == 68453u);
}

TEST_CASE("Playerbots Emote non-empty kits parse round-trip (Layer 3 fixture)", "[playerbots][packet]")
{
    WorldPackets::Chat::Emote packet;
    packet.Guid = ObjectGuid::Create<HighGuid::Player>(7);
    packet.EmoteID = 10;
    packet.SequenceVariation = 3;
    packet.SpellVisualKitIDs = { 1001, 1002 };

    WorldPacket wire = BuildEmoteFixture(packet);

    Playerbots::PacketParse::EmotePayload parsed;
    REQUIRE(Playerbots::PacketParse::TryReadEmote(wire, parsed));

    CHECK(parsed.Guid == ObjectGuid::Create<HighGuid::Player>(7));
    CHECK(parsed.EmoteID == 10u);
    CHECK(parsed.SequenceVariation == 3);
    REQUIRE(parsed.SpellVisualKitIDs.size() == 2);
    CHECK(parsed.SpellVisualKitIDs[0] == 1001);
    CHECK(parsed.SpellVisualKitIDs[1] == 1002);
}

TEST_CASE("Playerbots Emote truncated payload fails Layer 1 loudly", "[playerbots][packet]")
{
    WorldPackets::Chat::Emote packet;
    packet.Guid = ObjectGuid::Create<HighGuid::Player>(1);
    packet.EmoteID = 1;
    packet.SpellVisualKitIDs = { 5 };

    WorldPacket wire = BuildEmoteFixture(packet);
    REQUIRE(wire.size() > 4);
    wire.resize(4);

    Playerbots::PacketParse::EmotePayload parsed;
    CHECK_FALSE(Playerbots::PacketParse::TryReadEmote(wire, parsed));
}
