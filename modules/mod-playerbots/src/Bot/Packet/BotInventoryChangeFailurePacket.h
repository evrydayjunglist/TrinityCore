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

#ifndef TRINITY_BOT_INVENTORY_CHANGE_FAILURE_PACKET_H
#define TRINITY_BOT_INVENTORY_CHANGE_FAILURE_PACKET_H

#include "ItemDefines.h"
#include "ObjectGuid.h"
#include "WorldPacket.h"

namespace Playerbots::PacketParse
{
// Fields mirrored from WorldPackets::Item::InventoryChangeFailure::Write().
struct InventoryChangeFailurePayload
{
    InventoryResult BagResult = EQUIP_ERR_OK;
    ObjectGuid Item[2];
    uint8 ContainerBSlot = 0;
    int32 Level = 0;
    ObjectGuid SrcContainer;
    int32 SrcSlot = 0;
    ObjectGuid DstContainer;
    int32 LimitCategory = 0;
};

// Returns true on Layer-1 success (full consume, no exception). Does not perform Layer 2.
bool TryReadInventoryChangeFailure(WorldPacket const& packet, InventoryChangeFailurePayload& out);

// V1 TellMaster subset (AC-shaped strings; Midnight InventoryResult names). nullptr = no tell.
char const* LookupV1CannotEquipTell(InventoryResult result);
}

#endif
