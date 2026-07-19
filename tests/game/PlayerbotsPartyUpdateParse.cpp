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

#include "Bot/Packet/BotPartyUpdatePacket.h"
#include "Bot/Packet/BotPacketParse.h"
#include "Group.h"
#include "ObjectGuid.h"
#include "Opcodes.h"
#include "PartyPackets.h"
#include "SharedDefines.h"
#include "WorldPacket.h"

namespace
{
WorldPacket BuildPartyUpdateFixture(WorldPackets::Party::PartyUpdate& packet)
{
    WorldPacket const* written = packet.Write();
    WorldPacket copy(*written);
    copy.SetOpcode(SMSG_PARTY_UPDATE);
    return copy;
}
}

TEST_CASE("Playerbots PartyUpdate empty destroy parse round-trip (Layer 3 fixture)", "[playerbots][packet]")
{
    // Mirror Group::SendUpdateDestroyGroupToPlayer shape.
    WorldPackets::Party::PartyUpdate packet;
    packet.PartyFlags = GROUP_FLAG_DESTROYED;
    packet.PartyIndex = 0;
    packet.PartyType = GROUP_TYPE_NONE;
    packet.PartyGUID = ObjectGuid::Create<HighGuid::Party>(7);
    packet.MyIndex = -1;
    packet.SequenceNum = 3;

    WorldPacket wire = BuildPartyUpdateFixture(packet);

    Playerbots::PacketParse::PartyUpdatePayload parsed;
    REQUIRE(Playerbots::PacketParse::TryReadPartyUpdate(wire, parsed));

    CHECK(parsed.PartyFlags == GROUP_FLAG_DESTROYED);
    CHECK(parsed.PartyIndex == 0);
    CHECK(parsed.PartyType == GROUP_TYPE_NONE);
    CHECK(parsed.PartyGUID == ObjectGuid::Create<HighGuid::Party>(7));
    CHECK(parsed.MyIndex == -1);
    CHECK(parsed.SequenceNum == 3u);
    CHECK(parsed.LeaderGUID.IsEmpty());
    REQUIRE(parsed.PlayerList.empty());
    CHECK_FALSE(parsed.LootSettings.has_value());
    CHECK_FALSE(parsed.DifficultySettings.has_value());
    CHECK_FALSE(parsed.ChallengeMode.has_value());
    CHECK_FALSE(parsed.LfgInfos.has_value());
    CHECK(Playerbots::PacketParse::VERIFIED_BUILD == 68453u);
}

TEST_CASE("Playerbots PartyUpdate multi-member parse round-trip (Layer 3 fixture)", "[playerbots][packet]")
{
    ObjectGuid const partyGuid = ObjectGuid::Create<HighGuid::Party>(42);
    ObjectGuid const leaderGuid = ObjectGuid::Create<HighGuid::Player>(100);
    ObjectGuid const memberGuid = ObjectGuid::Create<HighGuid::Player>(101);

    WorldPackets::Party::PartyUpdate packet;
    packet.PartyFlags = 0;
    packet.PartyIndex = 0;
    packet.PartyType = GROUP_TYPE_NORMAL;
    packet.MyIndex = 0;
    packet.PartyGUID = partyGuid;
    packet.SequenceNum = 11;
    packet.LeaderGUID = leaderGuid;
    packet.LeaderFactionGroup = 1;
    packet.PingRestriction = RestrictPingsTo::None;

    WorldPackets::Party::PartyPlayerInfo& self = packet.PlayerList.emplace_back();
    self.GUID = leaderGuid;
    self.Name = "Leader";
    self.Class = CLASS_WARRIOR;
    self.FactionGroup = 1;
    self.Connected = true;
    self.Subgroup = 0;
    self.Flags = 0;
    self.RolesAssigned = 0;

    WorldPackets::Party::PartyPlayerInfo& other = packet.PlayerList.emplace_back();
    other.GUID = memberGuid;
    other.Name = "Member";
    other.Class = CLASS_MAGE;
    other.FactionGroup = 1;
    other.Connected = true;
    other.Subgroup = 0;
    other.Flags = 0;
    other.RolesAssigned = 0;

    packet.LootSettings.emplace();
    packet.LootSettings->Method = 0;
    packet.LootSettings->Threshold = ITEM_QUALITY_UNCOMMON;
    packet.LootSettings->LootMaster = ObjectGuid::Empty;

    packet.DifficultySettings.emplace();
    packet.DifficultySettings->DungeonDifficultyID = 1;
    packet.DifficultySettings->RaidDifficultyID = 14;
    packet.DifficultySettings->LegacyRaidDifficultyID = 3;

    WorldPacket wire = BuildPartyUpdateFixture(packet);

    Playerbots::PacketParse::PartyUpdatePayload parsed;
    REQUIRE(Playerbots::PacketParse::TryReadPartyUpdate(wire, parsed));

    CHECK(parsed.PartyFlags == 0);
    CHECK(parsed.PartyType == GROUP_TYPE_NORMAL);
    CHECK(parsed.MyIndex == 0);
    CHECK(parsed.PartyGUID == partyGuid);
    CHECK(parsed.SequenceNum == 11u);
    CHECK(parsed.LeaderGUID == leaderGuid);
    CHECK(parsed.LeaderFactionGroup == 1);
    REQUIRE(parsed.PlayerList.size() == 2);
    CHECK(parsed.PlayerList[0].GUID == leaderGuid);
    CHECK(parsed.PlayerList[0].Name == "Leader");
    CHECK(parsed.PlayerList[0].Class == CLASS_WARRIOR);
    CHECK(parsed.PlayerList[0].Connected == true);
    CHECK(parsed.PlayerList[1].GUID == memberGuid);
    CHECK(parsed.PlayerList[1].Name == "Member");
    CHECK(parsed.PlayerList[1].Class == CLASS_MAGE);
    REQUIRE(parsed.LootSettings.has_value());
    CHECK(parsed.LootSettings->Threshold == ITEM_QUALITY_UNCOMMON);
    REQUIRE(parsed.DifficultySettings.has_value());
    CHECK(parsed.DifficultySettings->DungeonDifficultyID == 1);
    CHECK(parsed.DifficultySettings->RaidDifficultyID == 14);
    CHECK_FALSE(parsed.ChallengeMode.has_value());
    CHECK_FALSE(parsed.LfgInfos.has_value());
    CHECK(Playerbots::PacketParse::VERIFIED_BUILD == 68453u);
}

TEST_CASE("Playerbots PartyUpdate truncated payload fails Layer 1 loudly", "[playerbots][packet]")
{
    WorldPackets::Party::PartyUpdate packet;
    packet.PartyFlags = GROUP_FLAG_DESTROYED;
    packet.PartyGUID = ObjectGuid::Create<HighGuid::Party>(1);
    packet.MyIndex = -1;
    packet.SequenceNum = 1;

    WorldPackets::Party::PartyPlayerInfo& filler = packet.PlayerList.emplace_back();
    filler.GUID = ObjectGuid::Create<HighGuid::Player>(2);
    filler.Name = "X";
    filler.Class = CLASS_WARRIOR;
    filler.Connected = true;

    WorldPacket wire = BuildPartyUpdateFixture(packet);
    REQUIRE(wire.size() > 8);
    wire.resize(wire.size() / 2);

    Playerbots::PacketParse::PartyUpdatePayload parsed;
    CHECK_FALSE(Playerbots::PacketParse::TryReadPartyUpdate(wire, parsed));
}
