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
#include "Bot/Packet/BotTradeStatusPacket.h"
#include "ObjectGuid.h"
#include "Opcodes.h"
#include "SharedDefines.h"
#include "TradePackets.h"
#include "WorldPacket.h"

namespace
{
WorldPacket BuildTradeStatusFixture(WorldPackets::Trade::TradeStatus& packet)
{
    WorldPacket const* written = packet.Write();
    WorldPacket copy(*written);
    copy.SetOpcode(SMSG_TRADE_STATUS);
    return copy;
}
}

TEST_CASE("Playerbots TradeStatus PROPOSED payload parse round-trip (Layer 3 fixture)", "[playerbots][packet]")
{
    WorldPackets::Trade::TradeStatus packet;
    packet.Status = TRADE_STATUS_PROPOSED;
    packet.Partner = ObjectGuid::Create<HighGuid::Player>(42);
    packet.PartnerAccount = ObjectGuid::Create<HighGuid::WowAccount>(7);
    packet.PartnerIsSameBnetAccount = true;

    WorldPacket wire = BuildTradeStatusFixture(packet);

    Playerbots::PacketParse::TradeStatusPayload parsed;
    REQUIRE(Playerbots::PacketParse::TryReadTradeStatus(wire, parsed));

    CHECK(parsed.Status == TRADE_STATUS_PROPOSED);
    CHECK(parsed.Partner == ObjectGuid::Create<HighGuid::Player>(42));
    CHECK(parsed.PartnerAccount == ObjectGuid::Create<HighGuid::WowAccount>(7));
    CHECK(parsed.PartnerIsSameBnetAccount == true);
    CHECK(Playerbots::PacketParse::IsKnownTradeStatus(parsed.Status));
    CHECK(Playerbots::PacketParse::VERIFIED_BUILD == 68453u);
}

TEST_CASE("Playerbots TradeStatus CANCELLED default trailer parse round-trip (Layer 3 fixture)", "[playerbots][packet]")
{
    WorldPackets::Trade::TradeStatus packet;
    packet.Status = TRADE_STATUS_CANCELLED;
    packet.PartnerIsSameBnetAccount = false;

    WorldPacket wire = BuildTradeStatusFixture(packet);

    Playerbots::PacketParse::TradeStatusPayload parsed;
    REQUIRE(Playerbots::PacketParse::TryReadTradeStatus(wire, parsed));

    CHECK(parsed.Status == TRADE_STATUS_CANCELLED);
    CHECK(parsed.PartnerIsSameBnetAccount == false);
    CHECK(Playerbots::PacketParse::VERIFIED_BUILD == 68453u);
}

TEST_CASE("Playerbots TradeStatus INITIATED trailer parse round-trip (Layer 3 fixture)", "[playerbots][packet]")
{
    WorldPackets::Trade::TradeStatus packet;
    packet.Status = TRADE_STATUS_INITIATED;
    packet.ID = 0xABCDEF01u;

    WorldPacket wire = BuildTradeStatusFixture(packet);

    Playerbots::PacketParse::TradeStatusPayload parsed;
    REQUIRE(Playerbots::PacketParse::TryReadTradeStatus(wire, parsed));

    CHECK(parsed.Status == TRADE_STATUS_INITIATED);
    CHECK(parsed.ID == 0xABCDEF01u);
    CHECK(Playerbots::PacketParse::VERIFIED_BUILD == 68453u);
}

TEST_CASE("Playerbots TradeStatus truncated payload fails Layer 1 loudly", "[playerbots][packet]")
{
    WorldPackets::Trade::TradeStatus packet;
    packet.Status = TRADE_STATUS_PROPOSED;
    packet.Partner = ObjectGuid::Create<HighGuid::Player>(99);

    WorldPacket wire = BuildTradeStatusFixture(packet);
    REQUIRE(wire.size() > 4);
    wire.resize(wire.size() / 2);

    Playerbots::PacketParse::TradeStatusPayload parsed;
    CHECK_FALSE(Playerbots::PacketParse::TryReadTradeStatus(wire, parsed));
}
