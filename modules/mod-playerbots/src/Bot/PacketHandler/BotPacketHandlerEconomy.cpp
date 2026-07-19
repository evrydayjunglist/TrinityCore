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
#include "Bot/Packet/BotBuyFailedPacket.h"
#include "Bot/Packet/BotInventoryChangeFailurePacket.h"
#include "Bot/Packet/BotTradeStatusPacket.h"
#include "Bot/Packet/BotTradeUpdatedPacket.h"
#include "Creature.h"
#include "DB2Stores.h"
#include "Item.h"
#include "ItemDefines.h"
#include "Map.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "TradeData.h"

namespace Playerbots::PacketHandler
{

namespace
{
bool IsKnownBuyResult(BuyResult reason)
{
    switch (reason)
    {
        case BUY_ERR_CANT_FIND_ITEM:
        case BUY_ERR_ITEM_ALREADY_SOLD:
        case BUY_ERR_NOT_ENOUGHT_MONEY:
        case BUY_ERR_SELLER_DONT_LIKE_YOU:
        case BUY_ERR_DISTANCE_TOO_FAR:
        case BUY_ERR_ITEM_SOLD_OUT:
        case BUY_ERR_CANT_CARRY_MORE:
        case BUY_ERR_RANK_REQUIRE:
        case BUY_ERR_REPUTATION_REQUIRE:
            return true;
        default:
            return false;
    }
}

bool IsKnownInventoryResult(int32 bagResult)
{
    return bagResult >= int32(EQUIP_ERR_OK) &&
        bagResult <= int32(EQUIP_ERR_NO_SALVAGED_ITEMS_IN_ACCOUNT_BANK);
}
}

void HandleBuyFailed(BotPlayerbotAI& ai, BotPlayerbotAI::QueuedSignal& signal)
{

        // AC registered BUY_ERR_* as fake opcodes; TC sends one SMSG_BUY_FAILED — dispatch
        // by Reason to AC signal names. Clear placeholder for non-V1 reasons.
        signal.Name.clear();

        Playerbots::PacketParse::BuyFailedPayload parsed;
        if (!Playerbots::PacketParse::TryReadBuyFailed(*signal.Packet, parsed))
            return;

        if (!IsKnownBuyResult(parsed.Reason))
        {
            TC_LOG_ERROR("playerbots.packet",
                "BotPacketParse SMSG_BUY_FAILED Layer-2 unknown Reason bot={} reason={} muid={} vendor={} verifiedBuild={}",
                ai.GetBot() ? ai.GetBot()->GetName() : "?",
                int32(parsed.Reason),
                parsed.Muid,
                parsed.VendorGUID.ToString(),
                Playerbots::PacketParse::VERIFIED_BUILD);
            return;
        }

        // V1 accept path: money / reputation only. Other known reasons parse-ok then ignore.
        if (parsed.Reason != BUY_ERR_NOT_ENOUGHT_MONEY && parsed.Reason != BUY_ERR_REPUTATION_REQUIRE)
        {
            if (Playerbots::GetLogLevel() >= 1)
                TC_LOG_DEBUG("playerbots.packet",
                    "BotPacketParse SMSG_BUY_FAILED ignored (non-V1 Reason) bot={} reason={} muid={}",
                    ai.GetBot() ? ai.GetBot()->GetName() : "?",
                    int32(parsed.Reason),
                    parsed.Muid);
            return;
        }

        Player* bot = ai.GetBot();
        if (parsed.Muid != 0)
        {
            bool const itemOk = sObjectMgr->GetItemTemplate(parsed.Muid) != nullptr;
            bool const currencyOk = sCurrencyTypesStore.LookupEntry(parsed.Muid) != nullptr;
            if (!itemOk && !currencyOk)
            {
                TC_LOG_ERROR("playerbots.packet",
                    "BotPacketParse SMSG_BUY_FAILED Layer-2 unknown Muid bot={} muid={} reason={} verifiedBuild={}",
                    bot ? bot->GetName() : "?",
                    parsed.Muid,
                    int32(parsed.Reason),
                    Playerbots::PacketParse::VERIFIED_BUILD);
                return;
            }
        }

        if (!parsed.VendorGUID.IsEmpty())
        {
            // Map-local only — do not cold-query remote maps (map-thread guardrails).
            Map* map = bot ? bot->GetMap() : nullptr;
            Creature* vendor = map ? map->GetCreature(parsed.VendorGUID) : nullptr;
            if (!vendor)
            {
                TC_LOG_ERROR("playerbots.packet",
                    "BotPacketParse SMSG_BUY_FAILED Layer-2 vendor not on bot map bot={} vendor={} reason={} verifiedBuild={}",
                    bot ? bot->GetName() : "?",
                    parsed.VendorGUID.ToString(),
                    int32(parsed.Reason),
                    Playerbots::PacketParse::VERIFIED_BUILD);
                return;
            }
        }

        signal.Name = (parsed.Reason == BUY_ERR_NOT_ENOUGHT_MONEY)
            ? "not enough money"
            : "not enough reputation";

        if (Playerbots::GetLogLevel() >= 1)
            TC_LOG_DEBUG("playerbots.packet",
                "BotPacketParse SMSG_BUY_FAILED ok bot={} signal='{}' vendor={} muid={} reason={}",
                bot ? bot->GetName() : "?",
                signal.Name,
                parsed.VendorGUID.ToString(),
                parsed.Muid,
                int32(parsed.Reason));
}

void HandleInventoryChangeFailure(BotPlayerbotAI& ai, BotPlayerbotAI::QueuedSignal& signal)
{

        // Cleared unless Layer 2 OK + V1 tell map hit — AC second name
        // ("inventory change failure") is the same reaction; no second FollowMaster trigger.
        signal.Name.clear();
        ai.ClearPendingCannotEquipTell();

        Playerbots::PacketParse::InventoryChangeFailurePayload parsed;
        if (!Playerbots::PacketParse::TryReadInventoryChangeFailure(*signal.Packet, parsed))
            return;

        if (parsed.BagResult == EQUIP_ERR_OK)
        {
            if (Playerbots::GetLogLevel() >= 1)
                TC_LOG_DEBUG("playerbots.packet",
                    "BotPacketParse SMSG_INVENTORY_CHANGE_FAILURE ignored (EQUIP_ERR_OK) bot={}",
                    ai.GetBot() ? ai.GetBot()->GetName() : "?");
            return;
        }

        if (!IsKnownInventoryResult(int32(parsed.BagResult)))
        {
            TC_LOG_ERROR("playerbots.packet",
                "BotPacketParse SMSG_INVENTORY_CHANGE_FAILURE Layer-2 unknown BagResult bot={} bagResult={} verifiedBuild={}",
                ai.GetBot() ? ai.GetBot()->GetName() : "?",
                int32(parsed.BagResult),
                Playerbots::PacketParse::VERIFIED_BUILD);
            return;
        }

        Player* bot = ai.GetBot();
        for (ObjectGuid const& itemGuid : parsed.Item)
        {
            if (itemGuid.IsEmpty())
                continue;

            // Map-local inventory only — do not cold-query remote maps.
            if (!bot || !bot->GetItemByGuid(itemGuid))
            {
                TC_LOG_ERROR("playerbots.packet",
                    "BotPacketParse SMSG_INVENTORY_CHANGE_FAILURE Layer-2 item not on bot bot={} item={} bagResult={} verifiedBuild={}",
                    bot ? bot->GetName() : "?",
                    itemGuid.ToString(),
                    int32(parsed.BagResult),
                    Playerbots::PacketParse::VERIFIED_BUILD);
                return;
            }
        }

        char const* tell = Playerbots::PacketParse::LookupV1CannotEquipTell(parsed.BagResult);
        if (!tell)
        {
            if (Playerbots::GetLogLevel() >= 1)
                TC_LOG_DEBUG("playerbots.packet",
                    "BotPacketParse SMSG_INVENTORY_CHANGE_FAILURE ok (no V1 tell) bot={} bagResult={}",
                    bot ? bot->GetName() : "?",
                    int32(parsed.BagResult));
            return;
        }

        ai.SetPendingCannotEquipTell(tell);
        signal.Name = "cannot equip";

        if (Playerbots::GetLogLevel() >= 1)
            TC_LOG_DEBUG("playerbots.packet",
                "BotPacketParse SMSG_INVENTORY_CHANGE_FAILURE ok bot={} bagResult={} tell='{}' item0={} item1={} level={} limitCategory={}",
                bot ? bot->GetName() : "?",
                int32(parsed.BagResult),
                tell,
                parsed.Item[0].ToString(),
                parsed.Item[1].ToString(),
                parsed.Level,
                parsed.LimitCategory);
}

void HandleTradeStatus(BotPlayerbotAI& ai, BotPlayerbotAI::QueuedSignal& signal)
{

        // Cleared unless Layer 2 OK + V1 Status (PROPOSED / ACCEPTED). Other statuses
        // parse-ok DEBUG only (COMPLETE/CANCELLED may race TradeData clear — OK).
        signal.Name.clear();
        ai.ClearPendingTradeStatus();

        Playerbots::PacketParse::TradeStatusPayload parsed;
        if (!Playerbots::PacketParse::TryReadTradeStatus(*signal.Packet, parsed))
            return;

        if (!Playerbots::PacketParse::IsKnownTradeStatus(parsed.Status))
        {
            TC_LOG_ERROR("playerbots.packet",
                "BotPacketParse SMSG_TRADE_STATUS Layer-2 unknown Status bot={} status={} verifiedBuild={}",
                ai.GetBot() ? ai.GetBot()->GetName() : "?",
                uint32(parsed.Status),
                Playerbots::PacketParse::VERIFIED_BUILD);
            return;
        }

        Player* bot = ai.GetBot();
        TradeData* trade = bot ? bot->GetTradeData() : nullptr;
        Player* trader = trade ? trade->GetTrader() : nullptr;

        if (parsed.Status == TRADE_STATUS_PROPOSED)
        {
            if (parsed.Partner.IsEmpty() || !trade || !trader ||
                parsed.Partner != trader->GetGUID())
            {
                TC_LOG_ERROR("playerbots.packet",
                    "BotPacketParse SMSG_TRADE_STATUS Layer-2 PROPOSED mismatch bot={} partner={} liveTrader={} verifiedBuild={}",
                    bot ? bot->GetName() : "?",
                    parsed.Partner.ToString(),
                    trader ? trader->GetGUID().ToString() : "none",
                    Playerbots::PacketParse::VERIFIED_BUILD);
                return;
            }

            ai.SetPendingTradeStatus(TRADE_STATUS_PROPOSED);
            signal.Name = "trade status";
        }
        else if (parsed.Status == TRADE_STATUS_ACCEPTED)
        {
            // Prefer live TradeData while accept is in flight; if already cleared, parse-only.
            if (!trade || !trader)
            {
                if (Playerbots::GetLogLevel() >= 1)
                    TC_LOG_DEBUG("playerbots.packet",
                        "BotPacketParse SMSG_TRADE_STATUS ok (ACCEPTED, no live trade) bot={}",
                        bot ? bot->GetName() : "?");
                return;
            }

            ai.SetPendingTradeStatus(TRADE_STATUS_ACCEPTED);
            signal.Name = "trade status";
        }
        else
        {
            // INITIATED / COMPLETE / CANCELLED / … — Layer-2 OK when Status known.
            // TradeData may already be gone on COMPLETE/CANCELLED; do not Layer-2 ERROR.
            if (Playerbots::GetLogLevel() >= 1)
                TC_LOG_DEBUG("playerbots.packet",
                    "BotPacketParse SMSG_TRADE_STATUS ok (no V1 AI) bot={} status={} hasTrade={}",
                    bot ? bot->GetName() : "?",
                    uint32(parsed.Status),
                    trade ? "yes" : "no");
            return;
        }

        if (Playerbots::GetLogLevel() >= 1)
            TC_LOG_DEBUG("playerbots.packet",
                "BotPacketParse SMSG_TRADE_STATUS ok bot={} status={} partner={} sameBnet={}",
                bot ? bot->GetName() : "?",
                uint32(parsed.Status),
                parsed.Partner.ToString(),
                parsed.PartnerIsSameBnetAccount ? "yes" : "no");
}

void HandleTradeUpdated(BotPlayerbotAI& ai, BotPlayerbotAI::QueuedSignal& signal)
{

        // Cleared unless Layer 2 OK. Enable=0 / mismatch: no poll dual for item/gold list.
        signal.Name.clear();
        ai.ClearPendingTradeUpdatedLockedTell();

        Playerbots::PacketParse::TradeUpdatedPayload parsed;
        if (!Playerbots::PacketParse::TryReadTradeUpdated(*signal.Packet, parsed))
            return;

        Player* bot = ai.GetBot();
        TradeData* trade = bot ? bot->GetTradeData() : nullptr;
        if (!trade)
        {
            TC_LOG_ERROR("playerbots.packet",
                "BotPacketParse SMSG_TRADE_UPDATED Layer-2 no trade bot={} whichPlayer={} verifiedBuild={}",
                bot ? bot->GetName() : "?",
                uint32(parsed.WhichPlayer),
                Playerbots::PacketParse::VERIFIED_BUILD);
            return;
        }

        TradeData* view = parsed.WhichPlayer != 0 ? trade->GetTraderData() : trade;
        if (!view)
        {
            TC_LOG_ERROR("playerbots.packet",
                "BotPacketParse SMSG_TRADE_UPDATED Layer-2 no view TradeData bot={} whichPlayer={} verifiedBuild={}",
                bot->GetName(),
                uint32(parsed.WhichPlayer),
                Playerbots::PacketParse::VERIFIED_BUILD);
            return;
        }

        if (parsed.Gold != view->GetMoney() ||
            parsed.CurrentStateIndex != view->GetServerStateIndex())
        {
            TC_LOG_ERROR("playerbots.packet",
                "BotPacketParse SMSG_TRADE_UPDATED Layer-2 mismatch bot={} whichPlayer={} parsedGold={} liveGold={} parsedState={} liveState={} verifiedBuild={}",
                bot->GetName(),
                uint32(parsed.WhichPlayer),
                parsed.Gold,
                view->GetMoney(),
                parsed.CurrentStateIndex,
                view->GetServerStateIndex(),
                Playerbots::PacketParse::VERIFIED_BUILD);
            return;
        }

        // Soft item dual: occupied parsed slots must match live view ItemID/stack.
        for (WorldPackets::Trade::TradeItem const& tradeItem : parsed.Items)
        {
            if (tradeItem.Slot >= TRADE_SLOT_COUNT)
            {
                TC_LOG_ERROR("playerbots.packet",
                    "BotPacketParse SMSG_TRADE_UPDATED Layer-2 bad Slot bot={} slot={} verifiedBuild={}",
                    bot->GetName(),
                    uint32(tradeItem.Slot),
                    Playerbots::PacketParse::VERIFIED_BUILD);
                return;
            }

            Item* liveItem = view->GetItem(TradeSlots(tradeItem.Slot));
            if (!liveItem || liveItem->GetEntry() != tradeItem.Item.ItemID ||
                int32(liveItem->GetCount()) != tradeItem.StackCount)
            {
                TC_LOG_ERROR("playerbots.packet",
                    "BotPacketParse SMSG_TRADE_UPDATED Layer-2 item mismatch bot={} slot={} parsedItem={} parsedStack={} liveItem={} liveStack={} verifiedBuild={}",
                    bot->GetName(),
                    uint32(tradeItem.Slot),
                    tradeItem.Item.ItemID,
                    tradeItem.StackCount,
                    liveItem ? liveItem->GetEntry() : 0,
                    liveItem ? int32(liveItem->GetCount()) : 0,
                    Playerbots::PacketParse::VERIFIED_BUILD);
                return;
            }
        }

        // ClientStateIndex may race (soft) — log only when DEBUG and mismatched.
        if (Playerbots::GetLogLevel() >= 1 &&
            parsed.ClientStateIndex != view->GetClientStateIndex())
        {
            TC_LOG_DEBUG("playerbots.packet",
                "BotPacketParse SMSG_TRADE_UPDATED ClientStateIndex soft mismatch bot={} parsed={} live={}",
                bot->GetName(),
                parsed.ClientStateIndex,
                view->GetClientStateIndex());
        }

        signal.Name = "trade status extended";

        Player* master = ai.GetMaster();
        Player* trader = trade->GetTrader();
        bool lockedNonTraded = false;
        for (WorldPackets::Trade::TradeItem const& tradeItem : parsed.Items)
        {
            if (tradeItem.Slot == TRADE_SLOT_NONTRADED && tradeItem.Unwrapped &&
                tradeItem.Unwrapped->Lock)
            {
                lockedNonTraded = true;
                break;
            }
        }

        if (lockedNonTraded && master && trader == master)
            ai.SetPendingTradeUpdatedLockedTell(true);

        if (Playerbots::GetLogLevel() >= 1)
            TC_LOG_DEBUG("playerbots.packet",
                "BotPacketParse SMSG_TRADE_UPDATED ok bot={} whichPlayer={} gold={} state={} items={} lockedNonTradedTell={}",
                bot->GetName(),
                uint32(parsed.WhichPlayer),
                parsed.Gold,
                parsed.CurrentStateIndex,
                parsed.Items.size(),
                ai.GetPendingTradeUpdatedLockedTell() ? "yes" : "no");
}

} // namespace Playerbots::PacketHandler
