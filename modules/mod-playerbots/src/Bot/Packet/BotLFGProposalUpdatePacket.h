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

#ifndef TRINITY_BOT_LFG_PROPOSAL_UPDATE_PACKET_H
#define TRINITY_BOT_LFG_PROPOSAL_UPDATE_PACKET_H

#include "LFGPacketsCommon.h"
#include "WorldPacket.h"
#include <vector>

namespace Playerbots::PacketParse
{
// Fields mirrored from WorldPackets::LFG::LFGProposalUpdate::Write() — module-first dual reader.
struct LFGProposalUpdatePlayerPayload
{
    uint8 Roles = 0;
    bool Me = false;
    bool SameParty = false;
    bool MyParty = false;
    bool Responded = false;
    bool Accepted = false;
};

struct LFGProposalUpdatePayload
{
    WorldPackets::LFG::RideTicket Ticket;
    uint64 InstanceID = 0;
    uint32 ProposalID = 0;
    uint32 Slot = 0;
    int8 State = 0;
    uint32 CompletedMask = 0;
    uint32 EncounterMask = 0;
    uint8 PromisedShortageRolePriority = 0;
    bool ValidCompletedMask = false;
    bool ProposalSilent = false;
    bool FailedByMyParty = false;
    std::vector<LFGProposalUpdatePlayerPayload> Players;
};

// Returns true on Layer-1 success (full consume, no exception). Does not perform Layer 2.
bool TryReadLFGProposalUpdate(WorldPacket const& packet, LFGProposalUpdatePayload& out);
}

#endif
