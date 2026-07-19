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

#ifndef TRINITY_BOT_PARTY_UPDATE_PACKET_H
#define TRINITY_BOT_PARTY_UPDATE_PACKET_H

#include "PartyPackets.h"
#include "WorldPacket.h"

namespace Playerbots::PacketParse
{
// Fields mirrored from WorldPackets::Party::PartyUpdate::Write() (+ nested PartyPlayerInfo /
// LeaverInfo + optional Loot/Difficulty/Challenge/LFG blocks). Nested types reuse TC structs.
struct PartyUpdatePayload
{
    uint16 PartyFlags = 0;
    uint8 PartyIndex = 0;
    uint8 PartyType = 0;
    int32 MyIndex = 0;
    ObjectGuid PartyGUID;
    uint32 SequenceNum = 0;
    ObjectGuid LeaderGUID;
    uint8 LeaderFactionGroup = 0;
    int32 PingRestriction = 0;
    std::vector<WorldPackets::Party::PartyPlayerInfo> PlayerList;
    Optional<WorldPackets::Party::ChallengeModeData> ChallengeMode;
    Optional<WorldPackets::Party::PartyLFGInfo> LfgInfos;
    Optional<WorldPackets::Party::PartyLootSettings> LootSettings;
    Optional<WorldPackets::Party::PartyDifficultySettings> DifficultySettings;
};

// Returns true on Layer-1 success (full consume, no exception). Does not perform Layer 2.
bool TryReadPartyUpdate(WorldPacket const& packet, PartyUpdatePayload& out);
}

#endif
