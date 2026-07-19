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

#include "Bot/Packet/BotMoveSetRunSpeedPacket.h"
#include "Bot/Packet/BotPacketParse.h"
#include "MovementPackets.h"
#include "ObjectGuid.h"
#include "Opcodes.h"
#include "WorldPacket.h"

namespace
{
WorldPacket BuildMoveSetRunSpeedFixture(ObjectGuid mover, uint32 sequenceIndex, float speed)
{
    WorldPackets::Movement::MoveSetSpeed packet(SMSG_MOVE_SET_RUN_SPEED);
    packet.MoverGUID = mover;
    packet.SequenceIndex = sequenceIndex;
    packet.Speed = speed;

    WorldPacket const* written = packet.Write();
    WorldPacket copy(*written);
    copy.SetOpcode(SMSG_MOVE_SET_RUN_SPEED);
    return copy;
}
}

TEST_CASE("Playerbots MoveSetRunSpeed payload parse round-trip (Layer 3 fixture)", "[playerbots][packet]")
{
    ObjectGuid const mover = ObjectGuid::Create<HighGuid::Player>(42);
    WorldPacket packet = BuildMoveSetRunSpeedFixture(mover, 7u, 14.0f);

    Playerbots::PacketParse::MoveSetRunSpeedPayload parsed;
    REQUIRE(Playerbots::PacketParse::TryReadMoveSetRunSpeed(packet, parsed));

    CHECK(parsed.MoverGUID == mover);
    CHECK(parsed.SequenceIndex == 7u);
    CHECK(parsed.Speed == 14.0f);
    CHECK(Playerbots::PacketParse::VERIFIED_BUILD == 68453u);
}

TEST_CASE("Playerbots MoveSetRunSpeed truncated payload fails Layer 1 loudly", "[playerbots][packet]")
{
    WorldPacket packet = BuildMoveSetRunSpeedFixture(ObjectGuid::Create<HighGuid::Player>(42), 1u, 7.0f);
    REQUIRE(packet.size() > 2);
    packet.resize(1);

    Playerbots::PacketParse::MoveSetRunSpeedPayload parsed;
    CHECK_FALSE(Playerbots::PacketParse::TryReadMoveSetRunSpeed(packet, parsed));
}
