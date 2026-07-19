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

#ifndef TRINITY_BOT_CAST_FAILED_PACKET_H
#define TRINITY_BOT_CAST_FAILED_PACKET_H

#include "Define.h"
#include "ObjectGuid.h"
#include "WorldPacket.h"

namespace Playerbots::PacketParse
{
// Fields mirrored from WorldPackets::Spells::CastFailed::Write()
// (incl. nested SpellCastVisual two×int32).
struct CastFailedVisual
{
    int32 SpellXSpellVisualID = 0;
    int32 ScriptVisualID = 0;
};

struct CastFailedPayload
{
    ObjectGuid CastID;
    int32 SpellID = 0;
    CastFailedVisual Visual;
    int32 Reason = 0;
    int32 FailedArg1 = -1;
    int32 FailedArg2 = -1;
    ObjectGuid FailedBy;
};

// Returns true on Layer-1 success (full consume, no exception). Does not perform Layer 2.
bool TryReadCastFailed(WorldPacket const& packet, CastFailedPayload& out);
}

#endif
