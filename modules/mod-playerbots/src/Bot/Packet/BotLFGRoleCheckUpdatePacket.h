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

#ifndef TRINITY_BOT_LFG_ROLE_CHECK_UPDATE_PACKET_H
#define TRINITY_BOT_LFG_ROLE_CHECK_UPDATE_PACKET_H

#include "ObjectGuid.h"
#include "WorldPacket.h"
#include <vector>

namespace Playerbots::PacketParse
{
// Fields mirrored from WorldPackets::LFG::LFGRoleCheckUpdate::Write() — module-first dual reader.
struct LFGRoleCheckUpdateMemberPayload
{
    ObjectGuid Guid;
    uint8 RolesDesired = 0;
    uint8 Level = 0;
    bool RoleCheckComplete = false;
};

struct LFGRoleCheckUpdatePayload
{
    uint8 PartyIndex = 0;
    uint8 RoleCheckStatus = 0;
    std::vector<uint32> JoinSlots;
    std::vector<uint64> BgQueueIDs;
    int32 GroupFinderActivityID = 0;
    std::vector<LFGRoleCheckUpdateMemberPayload> Members;
    bool IsBeginning = false;
    bool IsRequeue = false;
};

// Returns true on Layer-1 success (full consume, no exception). Does not perform Layer 2.
bool TryReadLFGRoleCheckUpdate(WorldPacket const& packet, LFGRoleCheckUpdatePayload& out);
}

#endif
