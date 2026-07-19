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

#include "BotPartyUpdatePacket.h"
#include "BotPacketParse.h"
#include "PacketOperators.h"

namespace Playerbots::PacketParse
{
namespace
{
void ReadLeaverInfo(ByteBuffer& data, WorldPackets::Party::LeaverInfo& out)
{
    // Mirror WorldPackets::Party::operator<<(LeaverInfo) field order exactly.
    data >> out.BnetAccountGUID;
    data >> out.LeaveScore;
    data >> out.SeasonID;
    data >> out.TotalLeaves;
    data >> out.TotalSuccesses;
    data >> out.ConsecutiveSuccesses;
    data >> out.LastPenaltyTime;
    data >> out.LeaverExpirationTime;
    data >> out.Unknown_1120;
    data >> WorldPackets::Bits<1>(out.LeaverStatus);
    data.ResetBitPos();
}

void ReadPartyPlayerInfo(ByteBuffer& data, WorldPackets::Party::PartyPlayerInfo& out)
{
    // Mirror WorldPackets::Party::operator<<(PartyPlayerInfo) field order exactly.
    data >> WorldPackets::SizedString::BitsSize<6>(out.Name);
    data >> WorldPackets::SizedCString::BitsSize<6>(out.VoiceStateID);
    data >> WorldPackets::Bits<1>(out.Connected);
    data >> WorldPackets::Bits<1>(out.VoiceChatSilenced);
    data >> WorldPackets::Bits<1>(out.FromSocialQueue);
    ReadLeaverInfo(data, out.Leaver);
    data >> out.GUID;
    data >> out.Subgroup;
    data >> out.Flags;
    data >> out.RolesAssigned;
    data >> out.Class;
    data >> out.FactionGroup;
    data >> WorldPackets::SizedString::Data(out.Name);
    data >> WorldPackets::SizedCString::Data(out.VoiceStateID);
}

void ReadPartyLootSettings(ByteBuffer& data, WorldPackets::Party::PartyLootSettings& out)
{
    data >> out.Method;
    data >> out.LootMaster;
    data >> out.Threshold;
}

void ReadPartyDifficultySettings(ByteBuffer& data, WorldPackets::Party::PartyDifficultySettings& out)
{
    data >> out.DungeonDifficultyID;
    data >> out.RaidDifficultyID;
    data >> out.LegacyRaidDifficultyID;
}

void ReadChallengeModeData(ByteBuffer& data, WorldPackets::Party::ChallengeModeData& out)
{
    // Mirror WorldPackets::Party::operator<<(ChallengeModeData) field order exactly.
    data >> out.MapID;
    data >> out.InitialPlayerCount;
    data >> out.InstanceID;
    data >> out.StartTime;
    data >> out.KeystoneOwnerGUID;
    data >> out.LeaverGUID;
    data >> out.InstanceAbandonVoteCooldown;
    data >> WorldPackets::Bits<1>(out.IsActive);
    data >> WorldPackets::Bits<1>(out.HasRestrictions);
    data >> WorldPackets::Bits<1>(out.CanVoteAbandon);
    data.ResetBitPos();
}

void ReadPartyLFGInfo(ByteBuffer& data, WorldPackets::Party::PartyLFGInfo& out)
{
    // Mirror WorldPackets::Party::operator<<(PartyLFGInfo) field order exactly.
    data >> out.Slot;
    data >> out.MyFlags;
    data >> out.MyRandomSlot;
    data >> out.MyPartialClear;
    data >> out.MyGearDiff;
    data >> out.MyStrangerCount;
    data >> out.MyKickVoteCount;
    data >> out.BootCount;
    data >> WorldPackets::Bits<1>(out.Aborted);
    data >> WorldPackets::Bits<1>(out.MyFirstReward);
    data.ResetBitPos();
}

void ReadPartyUpdateBody(ByteBuffer& data, PartyUpdatePayload& out)
{
    // Mirror WorldPackets::Party::PartyUpdate::Write() field order exactly.
    data >> out.PartyFlags;
    data >> out.PartyIndex;
    data >> out.PartyType;
    data >> out.MyIndex;
    data >> out.PartyGUID;
    data >> out.SequenceNum;
    data >> out.LeaderGUID;
    data >> out.LeaderFactionGroup;
    data >> out.PingRestriction;
    data >> WorldPackets::Size<uint32>(out.PlayerList);
    data >> WorldPackets::OptionalInit(out.ChallengeMode);
    data >> WorldPackets::OptionalInit(out.LfgInfos);
    data >> WorldPackets::OptionalInit(out.LootSettings);
    data >> WorldPackets::OptionalInit(out.DifficultySettings);
    data.ResetBitPos();

    for (WorldPackets::Party::PartyPlayerInfo& player : out.PlayerList)
        ReadPartyPlayerInfo(data, player);

    if (out.LootSettings)
        ReadPartyLootSettings(data, *out.LootSettings);

    if (out.DifficultySettings)
        ReadPartyDifficultySettings(data, *out.DifficultySettings);

    if (out.ChallengeMode)
        ReadChallengeModeData(data, *out.ChallengeMode);

    if (out.LfgInfos)
        ReadPartyLFGInfo(data, *out.LfgInfos);
}
}

bool TryReadPartyUpdate(WorldPacket const& packet, PartyUpdatePayload& out)
{
    PartyUpdatePayload parsed;
    Result const result = TryReadFully(packet, "SMSG_PARTY_UPDATE", [&parsed](WorldPacket& copy)
    {
        ReadPartyUpdateBody(copy, parsed);
    });

    if (result != Result::Ok)
        return false;

    out = std::move(parsed);
    return true;
}
}
