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

#include "Bot/PacketHandler/BotPacketSignal.h"
#include "Bot/Packet/BotPacketParse.h"
#include "Log.h"
#include "Opcodes.h"
#include "PlayerbotsConfig.h"
#include "Bot/Packet/BotLootResponsePacket.h"
#include "Bot/Packet/BotItemPushResultPacket.h"
#include "Bot/Packet/BotLootRollWonPacket.h"
#include "Bot/Packet/BotStartLootRollPacket.h"
#include "Group.h"
#include "Item.h"
#include "Loot.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "Player.h"

namespace Playerbots::PacketHandler
{
void HandleLootResponse(BotPlayerbotAI& ai, BotPlayerbotAI::QueuedSignal& signal)
{

        // Cleared unless Layer 2 OK + Acquired. Enable=0 / !Acquired / mismatch: no poll dual.
        signal.Name.clear();
        ai.ClearPendingLootStore();

        Playerbots::PacketParse::LootResponsePayload parsed;
        if (!Playerbots::PacketParse::TryReadLootResponse(*signal.Packet, parsed))
            return;

        if (!Playerbots::PacketParse::IsKnownLootError(parsed.FailureReason))
        {
            TC_LOG_ERROR("playerbots.packet",
                "BotPacketParse SMSG_LOOT_RESPONSE Layer-1 unknown FailureReason bot={} reason={} verifiedBuild={}",
                ai.GetBot() ? ai.GetBot()->GetName() : "?",
                uint32(parsed.FailureReason),
                Playerbots::PacketParse::VERIFIED_BUILD);
            return;
        }

        // Failure / no-loot path: parse-ok, no store, no Layer-2 ERROR.
        if (!parsed.Acquired)
        {
            if (Playerbots::GetLogLevel() >= 1)
                TC_LOG_DEBUG("playerbots.packet",
                    "BotPacketParse SMSG_LOOT_RESPONSE ok (!Acquired) bot={} owner={} reason={}",
                    ai.GetBot() ? ai.GetBot()->GetName() : "?",
                    parsed.Owner.ToString(),
                    uint32(parsed.FailureReason));
            return;
        }

        Player* bot = ai.GetBot();
        if (!bot)
            return;

        // GetLootByWorldObjectGUID matches Loot::GetOwnerGUID (parsed.Owner), not LootObj key.
        bool const guidDual =
            bot->GetLootGUID() == parsed.Owner ||
            bot->GetAELootView().contains(parsed.LootObj) ||
            bot->GetLootByWorldObjectGUID(parsed.Owner) != nullptr;

        if (!guidDual)
        {
            TC_LOG_ERROR("playerbots.packet",
                "BotPacketParse SMSG_LOOT_RESPONSE Layer-2 mismatch bot={} owner={} lootObj={} lootGuid={} verifiedBuild={}",
                bot->GetName(),
                parsed.Owner.ToString(),
                parsed.LootObj.ToString(),
                bot->GetLootGUID().ToString(),
                Playerbots::PacketParse::VERIFIED_BUILD);
            return;
        }

        BotPlayerbotAI::PendingLootStore stash;
        stash.Owner = parsed.Owner;
        stash.LootObj = parsed.LootObj;
        stash.Coins = parsed.Coins;
        stash.Items.reserve(parsed.Items.size());
        for (WorldPackets::Loot::LootItemData const& item : parsed.Items)
        {
            BotPlayerbotAI::PendingLootStore::ItemSlot slot;
            slot.LootListID = item.LootListID;
            slot.ItemID = item.Loot.ItemID;
            slot.UIType = item.UIType;
            stash.Items.push_back(slot);
        }

        ai.SetPendingLootStore(std::move(stash));
        signal.Name = "loot response";

        if (Playerbots::GetLogLevel() >= 1)
            TC_LOG_DEBUG("playerbots.packet",
                "BotPacketParse SMSG_LOOT_RESPONSE ok bot={} owner={} lootObj={} coins={} items={} ae={}",
                bot->GetName(),
                parsed.Owner.ToString(),
                parsed.LootObj.ToString(),
                parsed.Coins,
                parsed.Items.size(),
                parsed.AELooting ? "yes" : "no");
}

void HandleItemPushResult(BotPlayerbotAI& ai, BotPlayerbotAI::QueuedSignal& signal)
{

        // Cleared unless Layer 2 OK on self. Enable=0 / party broadcast / mismatch: no poll dual.
        signal.Name.clear();
        ai.ClearPendingItemPush();

        Playerbots::PacketParse::ItemPushResultPayload parsed;
        if (!Playerbots::PacketParse::TryReadItemPushResult(*signal.Packet, parsed))
            return;

        Player* bot = ai.GetBot();
        if (!bot)
            return;

        // Party BroadcastPacket from SendNewItem: expected noise — ignore, no Layer-2 ERROR.
        if (parsed.PlayerGUID != bot->GetGUID())
        {
            if (Playerbots::GetLogLevel() >= 1)
                TC_LOG_DEBUG("playerbots.packet",
                    "BotPacketParse SMSG_ITEM_PUSH_RESULT ignore (non-self) bot={} player={}",
                    bot->GetName(),
                    parsed.PlayerGUID.ToString());
            return;
        }

        bool const quantityOk = parsed.Quantity >= 1 || (parsed.Created && parsed.Quantity >= 0);
        bool const itemGuidOk = parsed.ItemGUID.IsEmpty() || bot->GetItemByGuid(parsed.ItemGUID) != nullptr;
        bool const itemIdOk = parsed.Item.ItemID != 0;
        bool const countOk = !itemIdOk ||
            int32(bot->GetItemCount(parsed.Item.ItemID)) == parsed.QuantityInInventory;

        if (!quantityOk || !itemGuidOk || !itemIdOk || !countOk)
        {
            TC_LOG_ERROR("playerbots.packet",
                "BotPacketParse SMSG_ITEM_PUSH_RESULT Layer-2 mismatch bot={} itemId={} qty={} qtyInv={} itemGuid={} liveCount={} created={} verifiedBuild={}",
                bot->GetName(),
                parsed.Item.ItemID,
                parsed.Quantity,
                parsed.QuantityInInventory,
                parsed.ItemGUID.ToString(),
                itemIdOk ? bot->GetItemCount(parsed.Item.ItemID) : 0u,
                parsed.Created ? "yes" : "no",
                Playerbots::PacketParse::VERIFIED_BUILD);
            return;
        }

        BotPlayerbotAI::PendingItemPush stash;
        stash.ItemID = parsed.Item.ItemID;
        stash.ProxyItemID = parsed.ProxyItemID;
        stash.Quantity = parsed.Quantity;
        stash.QuantityInInventory = parsed.QuantityInInventory;
        ai.SetPendingItemPush(std::move(stash));
        signal.Name = "item push result";

        if (Playerbots::GetLogLevel() >= 1)
            TC_LOG_DEBUG("playerbots.packet",
                "BotPacketParse SMSG_ITEM_PUSH_RESULT ok bot={} itemId={} qty={} qtyInv={} proxy={} created={}",
                bot->GetName(),
                parsed.Item.ItemID,
                parsed.Quantity,
                parsed.QuantityInInventory,
                parsed.ProxyItemID,
                parsed.Created ? "yes" : "no");
}

void HandleLootRollWon(BotPlayerbotAI& ai, BotPlayerbotAI::QueuedSignal& signal)
{

        // Cleared unless Layer 2 OK. Enable=0 / mismatch: no poll dual (roll usually already gone).
        signal.Name.clear();
        ai.ClearPendingLootRollWon();

        Playerbots::PacketParse::LootRollWonPayload parsed;
        if (!Playerbots::PacketParse::TryReadLootRollWon(*signal.Packet, parsed))
            return;

        if (!Playerbots::PacketParse::IsKnownLootRollWonRollType(parsed.RollType))
        {
            TC_LOG_ERROR("playerbots.packet",
                "BotPacketParse SMSG_LOOT_ROLL_WON Layer-2 unknown RollType bot={} rollType={} verifiedBuild={}",
                ai.GetBot() ? ai.GetBot()->GetName() : "?",
                uint32(parsed.RollType),
                Playerbots::PacketParse::VERIFIED_BUILD);
            return;
        }

        if (parsed.Winner.IsEmpty())
        {
            TC_LOG_ERROR("playerbots.packet",
                "BotPacketParse SMSG_LOOT_ROLL_WON Layer-2 empty Winner bot={} lootObj={} verifiedBuild={}",
                ai.GetBot() ? ai.GetBot()->GetName() : "?",
                parsed.LootObj.ToString(),
                Playerbots::PacketParse::VERIFIED_BUILD);
            return;
        }

        Player* bot = ai.GetBot();
        if (!bot)
            return;

        bool const liveRoll = bot->GetLootRoll(parsed.LootObj, parsed.Item.LootListID) != nullptr;
        bool const selfWinner = parsed.Winner == bot->GetGUID();
        Group const* group = bot->GetGroup();
        bool const groupDual = group && (selfWinner || group->IsMember(parsed.Winner));
        if (!liveRoll && !groupDual && !selfWinner)
        {
            TC_LOG_ERROR("playerbots.packet",
                "BotPacketParse SMSG_LOOT_ROLL_WON Layer-2 mismatch bot={} winner={} lootObj={} listId={} inGroup={} verifiedBuild={}",
                bot->GetName(),
                parsed.Winner.ToString(),
                parsed.LootObj.ToString(),
                uint32(parsed.Item.LootListID),
                group ? "yes" : "no",
                Playerbots::PacketParse::VERIFIED_BUILD);
            return;
        }

        // Soft ItemID: zero is unusual but not a hard Layer-2 fail (currency/edge).
        BotPlayerbotAI::PendingLootRollWon stash;
        stash.Winner = parsed.Winner;
        stash.ItemID = parsed.Item.Loot.ItemID;
        ai.SetPendingLootRollWon(std::move(stash));
        signal.Name = "loot roll won";

        if (Playerbots::GetLogLevel() >= 1)
            TC_LOG_DEBUG("playerbots.packet",
                "BotPacketParse SMSG_LOOT_ROLL_WON ok bot={} winner={} itemId={} roll={} rollType={} uiType={} self={}",
                bot->GetName(),
                parsed.Winner.ToString(),
                parsed.Item.Loot.ItemID,
                parsed.Roll,
                uint32(parsed.RollType),
                uint32(parsed.Item.UIType),
                selfWinner ? "yes" : "no");
}

void HandleStartLootRoll(BotPlayerbotAI& ai, BotPlayerbotAI::QueuedSignal& signal)
{

        // Cleared unless Layer 2 OK. Enable=0 / mismatch: no poll dual.
        signal.Name.clear();
        ai.ClearPendingMasterLootRoll();

        Playerbots::PacketParse::StartLootRollPayload parsed;
        if (!Playerbots::PacketParse::TryReadStartLootRoll(*signal.Packet, parsed))
            return;

        if (!Playerbots::PacketParse::IsStartLootRollMethod(parsed.Method))
        {
            TC_LOG_ERROR("playerbots.packet",
                "BotPacketParse SMSG_START_LOOT_ROLL Layer-2 unexpected Method bot={} method={} verifiedBuild={}",
                ai.GetBot() ? ai.GetBot()->GetName() : "?",
                uint32(parsed.Method),
                Playerbots::PacketParse::VERIFIED_BUILD);
            return;
        }

        Player* bot = ai.GetBot();
        if (!bot)
            return;

        if (!bot->GetLootRoll(parsed.LootObj, parsed.Item.LootListID))
        {
            TC_LOG_ERROR("playerbots.packet",
                "BotPacketParse SMSG_START_LOOT_ROLL Layer-2 no live GetLootRoll bot={} lootObj={} listId={} verifiedBuild={}",
                bot->GetName(),
                parsed.LootObj.ToString(),
                uint32(parsed.Item.LootListID),
                Playerbots::PacketParse::VERIFIED_BUILD);
            return;
        }

        // Soft duals (do not hard-fail): group, ItemID, MapID, ValidRolls.
        BotPlayerbotAI::PendingMasterLootRoll stash;
        stash.LootObj = parsed.LootObj;
        stash.LootListID = parsed.Item.LootListID;
        stash.ItemID = parsed.Item.Loot.ItemID;
        ai.SetPendingMasterLootRoll(std::move(stash));
        signal.Name = "master loot roll";

        if (Playerbots::GetLogLevel() >= 1)
            TC_LOG_DEBUG("playerbots.packet",
                "BotPacketParse SMSG_START_LOOT_ROLL ok bot={} lootObj={} listId={} itemId={} method={} validRolls={} mapId={} uiType={}",
                bot->GetName(),
                parsed.LootObj.ToString(),
                uint32(parsed.Item.LootListID),
                parsed.Item.Loot.ItemID,
                uint32(parsed.Method),
                uint32(parsed.ValidRolls),
                parsed.MapID,
                uint32(parsed.Item.UIType));
}

} // namespace Playerbots::PacketHandler
