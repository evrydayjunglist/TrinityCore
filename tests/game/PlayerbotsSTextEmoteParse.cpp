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
#include "Bot/Packet/BotSTextEmotePacket.h"
#include "ChatPackets.h"
#include "ObjectGuid.h"
#include "Opcodes.h"
#include "WorldPacket.h"

namespace
{
WorldPacket BuildSTextEmoteFixture(WorldPackets::Chat::STextEmote& packet)
{
    WorldPacket const* written = packet.Write();
    WorldPacket copy(*written);
    copy.SetOpcode(SMSG_TEXT_EMOTE);
    return copy;
}
}

TEST_CASE("Playerbots STextEmote parse round-trip (Layer 3 fixture)", "[playerbots][packet]")
{
    WorldPackets::Chat::STextEmote packet;
    packet.SourceGUID = ObjectGuid::Create<HighGuid::Player>(42);
    packet.SourceAccountGUID = ObjectGuid::Create<HighGuid::WowAccount>(7);
    packet.EmoteID = 4; // TEXT_EMOTE_WAVE shape (layout only)
    packet.SoundIndex = -1;
    packet.TargetGUID = ObjectGuid::Create<HighGuid::Player>(99);

    WorldPacket wire = BuildSTextEmoteFixture(packet);

    Playerbots::PacketParse::STextEmotePayload parsed;
    REQUIRE(Playerbots::PacketParse::TryReadSTextEmote(wire, parsed));

    CHECK(parsed.SourceGUID == ObjectGuid::Create<HighGuid::Player>(42));
    CHECK(parsed.SourceAccountGUID == ObjectGuid::Create<HighGuid::WowAccount>(7));
    CHECK(parsed.EmoteID == 4);
    CHECK(parsed.SoundIndex == -1);
    CHECK(parsed.TargetGUID == ObjectGuid::Create<HighGuid::Player>(99));
    CHECK(Playerbots::PacketParse::VERIFIED_BUILD == 68453u);
}

TEST_CASE("Playerbots STextEmote truncated payload fails Layer 1 loudly", "[playerbots][packet]")
{
    WorldPackets::Chat::STextEmote packet;
    packet.SourceGUID = ObjectGuid::Create<HighGuid::Player>(1);
    packet.EmoteID = 1;

    WorldPacket wire = BuildSTextEmoteFixture(packet);
    REQUIRE(wire.size() > 4);
    wire.resize(4);

    Playerbots::PacketParse::STextEmotePayload parsed;
    CHECK_FALSE(Playerbots::PacketParse::TryReadSTextEmote(wire, parsed));
}
