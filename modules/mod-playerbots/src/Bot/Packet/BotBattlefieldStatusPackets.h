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

#ifndef TRINITY_BOT_BATTLEFIELD_STATUS_PACKETS_H
#define TRINITY_BOT_BATTLEFIELD_STATUS_PACKETS_H

#include "BattlegroundPackets.h"
#include "WorldPacket.h"

namespace Playerbots::PacketParse
{
// Fields mirrored from WorldPackets::Battleground::BattlefieldStatus*::Write() —
// module-first dual readers for AC "bg status" (NeedConfirmation / Queued / Active).

struct BattlefieldStatusNeedConfirmationPayload
{
    WorldPackets::Battleground::BattlefieldStatusHeader Hdr;
    uint32 Mapid = 0;
    uint32 Timeout = 0;
    uint8 Role = 0;
};

struct BattlefieldStatusQueuedPayload
{
    WorldPackets::Battleground::BattlefieldStatusHeader Hdr;
    uint32 AverageWaitTime = 0;
    uint32 WaitTime = 0;
    int32 SpecSelected = 0;
    bool AsGroup = false;
    bool EligibleForMatchmaking = false;
    bool SuspendedQueue = false;
};

struct BattlefieldStatusActivePayload
{
    WorldPackets::Battleground::BattlefieldStatusHeader Hdr;
    uint32 Mapid = 0;
    uint32 ShutdownTimer = 0;
    uint32 StartTimer = 0;
    int8 ArenaFaction = 0;
    bool LeftEarly = false;
    bool Brawl = false;
};

// Returns true on Layer-1 success (full consume, no exception). Does not perform Layer 2.
bool TryReadBattlefieldStatusNeedConfirmation(WorldPacket const& packet, BattlefieldStatusNeedConfirmationPayload& out);
bool TryReadBattlefieldStatusQueued(WorldPacket const& packet, BattlefieldStatusQueuedPayload& out);
bool TryReadBattlefieldStatusActive(WorldPacket const& packet, BattlefieldStatusActivePayload& out);
}

#endif
