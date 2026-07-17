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

#include "Bot/Packet/BotBuyFailedPacket.h"
#include "Bot/Packet/BotPacketParse.h"
#include "ItemDefines.h"
#include "ItemPackets.h"
#include "ObjectGuid.h"
#include "Opcodes.h"
#include "WorldPacket.h"

namespace
{
WorldPacket BuildBuyFailedFixture(BuyResult reason, uint32 muid = 12345)
{
    WorldPackets::Item::BuyFailed packet;
    packet.VendorGUID = ObjectGuid::Create<HighGuid::Creature>(0, 500, 1);
    packet.Muid = muid;
    packet.Reason = reason;

    WorldPacket const* written = packet.Write();
    WorldPacket copy(*written);
    copy.SetOpcode(SMSG_BUY_FAILED);
    return copy;
}
}

TEST_CASE("Playerbots BuyFailed money-reason payload parse round-trip (Layer 3 fixture)", "[playerbots][packet]")
{
    WorldPacket packet = BuildBuyFailedFixture(BUY_ERR_NOT_ENOUGHT_MONEY);

    Playerbots::PacketParse::BuyFailedPayload parsed;
    REQUIRE(Playerbots::PacketParse::TryReadBuyFailed(packet, parsed));

    CHECK(parsed.VendorGUID == ObjectGuid::Create<HighGuid::Creature>(0, 500, 1));
    CHECK(parsed.Muid == 12345u);
    CHECK(parsed.Reason == BUY_ERR_NOT_ENOUGHT_MONEY);
    CHECK(Playerbots::PacketParse::VERIFIED_BUILD == 68453u);
}

TEST_CASE("Playerbots BuyFailed reputation-reason payload parse round-trip (Layer 3 fixture)", "[playerbots][packet]")
{
    WorldPacket packet = BuildBuyFailedFixture(BUY_ERR_REPUTATION_REQUIRE, 99);

    Playerbots::PacketParse::BuyFailedPayload parsed;
    REQUIRE(Playerbots::PacketParse::TryReadBuyFailed(packet, parsed));

    CHECK(parsed.VendorGUID == ObjectGuid::Create<HighGuid::Creature>(0, 500, 1));
    CHECK(parsed.Muid == 99u);
    CHECK(parsed.Reason == BUY_ERR_REPUTATION_REQUIRE);
    CHECK(Playerbots::PacketParse::VERIFIED_BUILD == 68453u);
}

TEST_CASE("Playerbots BuyFailed truncated payload fails Layer 1 loudly", "[playerbots][packet]")
{
    WorldPacket packet = BuildBuyFailedFixture(BUY_ERR_NOT_ENOUGHT_MONEY);
    REQUIRE(packet.size() > 4);
    packet.resize(packet.size() / 2);

    Playerbots::PacketParse::BuyFailedPayload parsed;
    CHECK_FALSE(Playerbots::PacketParse::TryReadBuyFailed(packet, parsed));
}
