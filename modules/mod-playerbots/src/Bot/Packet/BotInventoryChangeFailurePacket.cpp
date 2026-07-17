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

#include "BotInventoryChangeFailurePacket.h"
#include "BotPacketParse.h"

namespace Playerbots::PacketParse
{
namespace
{
void ReadInventoryChangeFailureBody(ByteBuffer& data, InventoryChangeFailurePayload& out)
{
    // Mirror WorldPackets::Item::InventoryChangeFailure::Write() field order exactly.
    int32 bagResult = 0;
    data >> bagResult;
    out.BagResult = InventoryResult(bagResult);
    data >> out.Item[0];
    data >> out.Item[1];
    data >> out.ContainerBSlot;

    switch (out.BagResult)
    {
        case EQUIP_ERR_CANT_EQUIP_LEVEL_I:
        case EQUIP_ERR_PURCHASE_LEVEL_TOO_LOW:
            data >> out.Level;
            break;
        case EQUIP_ERR_EVENT_AUTOEQUIP_BIND_CONFIRM:
            data >> out.SrcContainer;
            data >> out.SrcSlot;
            data >> out.DstContainer;
            break;
        case EQUIP_ERR_ITEM_MAX_LIMIT_CATEGORY_COUNT_EXCEEDED_IS:
        case EQUIP_ERR_ITEM_MAX_LIMIT_CATEGORY_SOCKETED_EXCEEDED_IS:
        case EQUIP_ERR_ITEM_MAX_LIMIT_CATEGORY_EQUIPPED_EXCEEDED_IS:
            data >> out.LimitCategory;
            break;
        default:
            break;
    }
}
}

bool TryReadInventoryChangeFailure(WorldPacket const& packet, InventoryChangeFailurePayload& out)
{
    InventoryChangeFailurePayload parsed;
    Result const result = TryReadFully(packet, "SMSG_INVENTORY_CHANGE_FAILURE", [&parsed](WorldPacket& copy)
    {
        ReadInventoryChangeFailureBody(copy, parsed);
    });

    if (result != Result::Ok)
        return false;

    out = parsed;
    return true;
}

char const* LookupV1CannotEquipTell(InventoryResult result)
{
    // Documented V1 subset — expand toward AC messages after Midnight enum audit.
    switch (result)
    {
        case EQUIP_ERR_CANT_EQUIP_LEVEL_I:
            return "My level is too low";
        case EQUIP_ERR_BAG_FULL:
            return "My bags are full";
        case EQUIP_ERR_INV_FULL:
            return "My inventory is full";
        case EQUIP_ERR_NOT_EQUIPPABLE:
            return "Item cannot be equipped";
        default:
            return nullptr;
    }
}
}
