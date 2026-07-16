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

#include "Bot/Packet/BotPartyInvitePacket.h"
#include "Bot/Packet/BotPacketParse.h"
#include "ObjectGuid.h"
#include "Opcodes.h"
#include "PartyPackets.h"
#include "WorldPacket.h"

namespace
{
WorldPacket BuildPartyInviteFixture()
{
    WorldPackets::Party::PartyInvite invite;
    invite.CanAccept = true;
    invite.IsXRealm = false;
    invite.IsXNativeRealm = true;
    invite.ShouldSquelch = false;
    invite.AllowMultipleRoles = false;
    invite.QuestSessionActive = false;
    invite.IsCrossFaction = false;
    invite.InviterRealm = WorldPackets::Auth::VirtualRealmInfo(
        0x01000001, true, false, "Trinity", "trinity");
    invite.InviterGUID = ObjectGuid::Create<HighGuid::Player>(42);
    invite.InviterBNetAccountId = ObjectGuid::Create<HighGuid::WowAccount>(7);
    invite.InviterCfgRealmID = 1;
    invite.ProposedRoles = 1;
    invite.LfgCompletedMask = 0;
    invite.LfgSlots = { 101, 202 };
    invite.InviterName = "Inviter";

    WorldPacket const* written = invite.Write();
    WorldPacket packet(*written);
    packet.SetOpcode(SMSG_PARTY_INVITE);
    return packet;
}
}

TEST_CASE("Playerbots PartyInvite payload parse round-trip (Layer 3 fixture)", "[playerbots][packet]")
{
    WorldPacket packet = BuildPartyInviteFixture();

    Playerbots::PacketParse::PartyInvitePayload parsed;
    REQUIRE(Playerbots::PacketParse::TryReadPartyInvite(packet, parsed));

    CHECK(parsed.CanAccept == true);
    CHECK(parsed.IsXNativeRealm == true);
    CHECK(parsed.InviterName == "Inviter");
    CHECK(parsed.InviterGUID == ObjectGuid::Create<HighGuid::Player>(42));
    CHECK(parsed.InviterBNetAccountId == ObjectGuid::Create<HighGuid::WowAccount>(7));
    CHECK(parsed.ProposedRoles == 1);
    CHECK(parsed.LfgSlots.size() == 2);
    CHECK(parsed.LfgSlots[0] == 101);
    CHECK(parsed.LfgSlots[1] == 202);
    CHECK(parsed.InviterRealm.RealmAddress == 0x01000001);
    CHECK(parsed.InviterRealm.RealmNameInfo.RealmNameActual == "Trinity");
    CHECK(Playerbots::PacketParse::VERIFIED_BUILD == 68453u);
}

TEST_CASE("Playerbots PartyInvite truncated payload fails Layer 1 loudly", "[playerbots][packet]")
{
    WorldPacket packet = BuildPartyInviteFixture();
    REQUIRE(packet.size() > 4);
    packet.resize(packet.size() / 2);

    Playerbots::PacketParse::PartyInvitePayload parsed;
    CHECK_FALSE(Playerbots::PacketParse::TryReadPartyInvite(packet, parsed));
}
