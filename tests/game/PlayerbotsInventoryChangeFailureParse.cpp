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

#include "Bot/Packet/BotInventoryChangeFailurePacket.h"
#include "Bot/Packet/BotPacketParse.h"
#include "ItemDefines.h"
#include "ItemPackets.h"
#include "ObjectGuid.h"
#include "Opcodes.h"
#include "WorldPacket.h"

namespace
{
WorldPacket BuildInventoryChangeFailureFixture(InventoryResult bagResult, int32 level = 0)
{
    WorldPackets::Item::InventoryChangeFailure packet;
    packet.BagResult = bagResult;
    packet.Item[0] = ObjectGuid::Create<HighGuid::Item>(100);
    packet.Item[1] = ObjectGuid::Empty;
    packet.ContainerBSlot = 0;
    packet.Level = level;

    WorldPacket const* written = packet.Write();
    WorldPacket copy(*written);
    copy.SetOpcode(SMSG_INVENTORY_CHANGE_FAILURE);
    return copy;
}
}

TEST_CASE("Playerbots InventoryChangeFailure bags-full payload parse round-trip (Layer 3 fixture)", "[playerbots][packet]")
{
    WorldPacket packet = BuildInventoryChangeFailureFixture(EQUIP_ERR_BAG_FULL);

    Playerbots::PacketParse::InventoryChangeFailurePayload parsed;
    REQUIRE(Playerbots::PacketParse::TryReadInventoryChangeFailure(packet, parsed));

    CHECK(parsed.BagResult == EQUIP_ERR_BAG_FULL);
    CHECK(parsed.Item[0] == ObjectGuid::Create<HighGuid::Item>(100));
    CHECK(parsed.Item[1].IsEmpty());
    CHECK(parsed.ContainerBSlot == 0);
    CHECK(Playerbots::PacketParse::LookupV1CannotEquipTell(parsed.BagResult) != nullptr);
    CHECK(Playerbots::PacketParse::VERIFIED_BUILD == 68453u);
}

TEST_CASE("Playerbots InventoryChangeFailure level trailer payload parse round-trip (Layer 3 fixture)", "[playerbots][packet]")
{
    WorldPacket packet = BuildInventoryChangeFailureFixture(EQUIP_ERR_CANT_EQUIP_LEVEL_I, 60);

    Playerbots::PacketParse::InventoryChangeFailurePayload parsed;
    REQUIRE(Playerbots::PacketParse::TryReadInventoryChangeFailure(packet, parsed));

    CHECK(parsed.BagResult == EQUIP_ERR_CANT_EQUIP_LEVEL_I);
    CHECK(parsed.Level == 60);
    CHECK(Playerbots::PacketParse::LookupV1CannotEquipTell(parsed.BagResult) != nullptr);
    CHECK(Playerbots::PacketParse::VERIFIED_BUILD == 68453u);
}

TEST_CASE("Playerbots InventoryChangeFailure truncated payload fails Layer 1 loudly", "[playerbots][packet]")
{
    WorldPacket packet = BuildInventoryChangeFailureFixture(EQUIP_ERR_BAG_FULL);
    REQUIRE(packet.size() > 4);
    packet.resize(packet.size() / 2);

    Playerbots::PacketParse::InventoryChangeFailurePayload parsed;
    CHECK_FALSE(Playerbots::PacketParse::TryReadInventoryChangeFailure(packet, parsed));
}
