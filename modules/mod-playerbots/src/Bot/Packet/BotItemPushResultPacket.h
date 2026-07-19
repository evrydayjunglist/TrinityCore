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

#ifndef TRINITY_BOT_ITEM_PUSH_RESULT_PACKET_H
#define TRINITY_BOT_ITEM_PUSH_RESULT_PACKET_H

#include "CraftingPacketsCommon.h"
#include "ItemPackets.h"
#include "ItemPacketsCommon.h"
#include "WorldPacket.h"
#include <vector>

namespace Playerbots::PacketParse
{
// Fields mirrored from WorldPackets::Item::ItemPushResult::Write() (+ ItemInstance / toasts / craft).
struct ItemPushResultPayload
{
    ObjectGuid PlayerGUID;
    uint8 Slot = 0;
    int32 SlotInBag = 0;
    int32 ProxyItemID = 0;
    int32 Quantity = 0;
    int32 QuantityInInventory = 0;
    int32 QuantityInQuestLog = 0;
    int32 EncounterID = 0;
    int32 BattlePetSpeciesID = 0;
    int32 BattlePetBreedID = 0;
    uint8 BattlePetBreedQuality = 0;
    int32 BattlePetLevel = 0;
    ObjectGuid ItemGUID;
    std::vector<WorldPackets::Item::UiEventToast> Toasts;
    WorldPackets::Item::ItemInstance Item;
    Optional<uint32> FirstCraftOperationID;
    Optional<WorldPackets::Crafting::CraftingData> CraftingData;
    bool Pushed = false;
    WorldPackets::Item::ItemPushResult::DisplayType ChatNotifyType =
        WorldPackets::Item::ItemPushResult::DISPLAY_TYPE_HIDDEN;
    bool Created = false;
    bool FakeQuestItem = false;
    bool IsBonusRoll = false;
    bool IsPersonalLoot = false;
};

// Returns true on Layer-1 success (full consume, no exception). Does not perform Layer 2.
bool TryReadItemPushResult(WorldPacket const& packet, ItemPushResultPayload& out);
}

#endif
