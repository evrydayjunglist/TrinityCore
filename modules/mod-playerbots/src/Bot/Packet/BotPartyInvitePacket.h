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

#ifndef TRINITY_BOT_PARTY_INVITE_PACKET_H
#define TRINITY_BOT_PARTY_INVITE_PACKET_H

#include "AuthenticationPackets.h"
#include "ObjectGuid.h"
#include "WorldPacket.h"
#include <string>
#include <vector>

namespace Playerbots::PacketParse
{
// Fields mirrored from WorldPackets::Party::PartyInvite::Write() — module-first dual reader.
// Intentional: will not compile-break when core Write() changes; Layer 3 fixtures catch drift.
struct PartyInvitePayload
{
    bool CanAccept = false;
    bool IsXRealm = false;
    bool IsXNativeRealm = false;
    bool ShouldSquelch = false;
    bool AllowMultipleRoles = false;
    bool QuestSessionActive = false;
    bool IsCrossFaction = false;

    WorldPackets::Auth::VirtualRealmInfo InviterRealm;
    ObjectGuid InviterGUID;
    ObjectGuid InviterBNetAccountId;
    uint16 InviterCfgRealmID = 0;
    uint8 ProposedRoles = 0;
    uint32 LfgCompletedMask = 0;
    std::vector<uint32> LfgSlots;
    std::string InviterName;
};

// Returns true on Layer-1 success (full consume, no exception). Does not perform Layer 2.
bool TryReadPartyInvite(WorldPacket const& packet, PartyInvitePayload& out);
}

#endif
