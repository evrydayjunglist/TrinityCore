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

#include "BotPlayerbotAI.h"
#include "AiFactory.h"
#include "Bot/Packet/BotBuyFailedPacket.h"
#include "Bot/Packet/BotInventoryChangeFailurePacket.h"
#include "Bot/Packet/BotMoveSetRunSpeedPacket.h"
#include "Bot/Packet/BotGroupNewLeaderPacket.h"
#include "Bot/Packet/BotGuildInvitePacket.h"
#include "Bot/Packet/BotPetitionShowSignaturesPacket.h"
#include "Bot/Packet/BotPartyInvitePacket.h"
#include "Bot/Packet/BotPacketParse.h"
#include "Bot/Packet/BotResurrectRequestPacket.h"
#include "Bot/Packet/BotTradeStatusPacket.h"
#include "TradeData.h"
#include "Creature.h"
#include "DB2Stores.h"
#include "Engine.h"
#include "Group.h"
#include "ItemDefines.h"
#include "Log.h"
#include "Map.h"
#include "ObjectMgr.h"
#include "Opcodes.h"
#include "PetitionMgr.h"
#include "Player.h"
#include "PlayerbotsConfig.h"
#include "Random.h"
#include "RandomPlayerbotMgr.h"
#include "SharedDefines.h"
#include "WorldPacket.h"
#include <algorithm>
#include <cmath>
#include <unordered_map>

namespace
{
// AC's botOutgoingPacketHandlers shape (mod-playerbots-master PlayerbotAI.cpp:178-223).
// Signal name always; payload parse is opt-in per opcode behind the three-layer gate
// (playerbots-bot-packet-payload-parse-handoff.md).
struct PacketSignalEntry
{
    char const* SignalName;
    bool CopiesPayload; // when PayloadParse.Enable, enqueue a WorldPacket copy for tick-time parse
};

PacketSignalEntry const* LookupPacketSignal(uint32 opcode)
{
    static std::unordered_map<uint32, PacketSignalEntry> const registry = {
        { SMSG_PARTY_INVITE, { "group invite signal", true } },
        { SMSG_GUILD_INVITE, { "guild invite signal", true } },
        { SMSG_RESURRECT_REQUEST, { "resurrect request signal", true } },
        { SMSG_PETITION_SHOW_SIGNATURES, { "petition offer signal", true } },
        // Placeholder name rewritten in ProcessPayloadOnTick to AC "not enough money" /
        // "not enough reputation" (or cleared for other BuyResult reasons).
        { SMSG_BUY_FAILED, { "buy failed", true } },
        // AC SMSG_GROUP_SET_LEADER → Midnight SMSG_GROUP_NEW_LEADER; Cleared unless Layer 2 OK
        // (and when PayloadParse is off — no poll dual for leader name alone).
        { SMSG_GROUP_NEW_LEADER, { "group set leader", true } },
        // AC SMSG_FORCE_RUN_SPEED_CHANGE → Midnight SMSG_MOVE_SET_RUN_SPEED; Cleared unless
        // Layer 2 OK (and when PayloadParse is off — no poll dual for "speed just changed").
        { SMSG_MOVE_SET_RUN_SPEED, { "check mount state", true } },
        // AC registered the same opcode twice ("cannot equip" / "inventory change failure").
        // One TC SMSG; V1 fires the strategy-wired name only. Cleared unless Layer 2 OK.
        { SMSG_INVENTORY_CHANGE_FAILURE, { "cannot equip", true } },
        // Cleared unless Layer 2 OK and Status is PROPOSED/ACCEPTED (V1 begin/accept).
        // Enable=0 / other statuses: no poll dual for trade window.
        { SMSG_TRADE_STATUS, { "trade status", true } },
    };

    auto itr = registry.find(opcode);
    return itr != registry.end() ? &itr->second : nullptr;
}

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

constexpr size_t BOT_SIGNAL_QUEUE_MAX = 32;
constexpr size_t BOT_SIGNAL_PENDING_MAX = 32;
constexpr uint8 BOT_SIGNAL_TTL_TICKS = 3;
}

BotPlayerbotAI::BotPlayerbotAI(Player* bot) : PlayerbotAIBase(bot)
{
    _context = AiFactory::CreateContext(this, bot);
    _engine = std::make_unique<Engine>(this, _context.get());
    ResetStrategies();
}

void BotPlayerbotAI::ResetStrategies()
{
    if (!_engine)
        return;

    _engine->RemoveAllStrategies();

    std::string appliedStrategies;
    if (HasMaster())
    {
        _engine->AddStrategy("follow");
        _engine->AddStrategy("attack");
        appliedStrategies = "follow,attack";
    }
    else if (Player* bot = GetBot(); bot && sRandomPlayerbotMgr->IsRandomBot(bot->GetGUID()) &&
        roll_chance(Playerbots::GetRandomBotRpgChance()))
    {
        _engine->AddStrategy("newrpg");
        appliedStrategies = "newrpg";
        _rpgInfo.Reset();
    }
    else
    {
        _engine->AddStrategy("passive");
        appliedStrategies = "passive";
    }

    _engine->Init();

    if (Playerbots::GetLogLevel() >= 1)
    {
        TC_LOG_DEBUG("playerbots", "BotPlayerbotAI::ResetStrategies bot={} master={} strategies={}",
            GetBot() ? GetBot()->GetName() : "?",
            HasMaster() ? "yes" : "no",
            appliedStrategies);
    }
}

void BotPlayerbotAI::HandleBotOutgoingPacket(WorldPacket const& packet)
{
    // Sender-thread cheap path: opcode registry + optional packet copy enqueue only.
    PacketSignalEntry const* entry = LookupPacketSignal(packet.GetOpcode());
    if (!entry)
        return;

    QueuedSignal queued;
    queued.Name = entry->SignalName;
    if (entry->CopiesPayload && Playerbots::GetPacketPayloadParseEnabled())
        queued.Packet = packet; // WorldPacket copy

    std::lock_guard<std::mutex> lock(_signalMutex);
    if (_signalQueue.size() >= BOT_SIGNAL_QUEUE_MAX)
    {
        _signalQueue.erase(_signalQueue.begin());
        if (Playerbots::GetLogLevel() >= 1)
            TC_LOG_DEBUG("playerbots", "BotPlayerbotAI signal queue overflow (dropped oldest) for bot {}",
                GetBot() ? GetBot()->GetName() : "?");
    }
    _signalQueue.push_back(std::move(queued));
    if (Playerbots::GetLogLevel() >= 1)
        TC_LOG_DEBUG("playerbots", "BotPlayerbotAI queued signal '{}' for bot {}",
            entry->SignalName, GetBot() ? GetBot()->GetName() : "?");
}

bool BotPlayerbotAI::ConsumeSignal(std::string const& name)
{
    auto itr = std::ranges::find_if(_pendingSignals, [&name](PendingSignal const& signal)
    {
        return signal.Name == name;
    });

    if (itr == _pendingSignals.end())
        return false;

    _pendingSignals.erase(itr);
    return true;
}

bool BotPlayerbotAI::TellMaster(std::string_view text)
{
    Player* bot = GetBot();
    Player* master = GetMaster();
    if (!bot || !master)
    {
        if (Playerbots::GetLogLevel() >= 1)
            TC_LOG_DEBUG("playerbots", "TellMaster skipped (no master) bot={} text='{}'",
                bot ? bot->GetName() : "?",
                text);
        return false;
    }

    bot->Whisper(text, LANG_UNIVERSAL, master);
    return true;
}

void BotPlayerbotAI::ProcessPayloadOnTick(QueuedSignal& signal)
{
    if (!signal.Packet)
    {
        // Parse-gated reactions with no poll dual: do not fire when PayloadParse is off.
        if (signal.Name == "group set leader" || signal.Name == "check mount state" ||
            signal.Name == "cannot equip" || signal.Name == "trade status")
        {
            if (signal.Name == "cannot equip")
                ClearPendingCannotEquipTell();
            if (signal.Name == "trade status")
                ClearPendingTradeStatus();
            signal.Name.clear();
        }
        return;
    }

    switch (signal.Packet->GetOpcode())
    {
        case SMSG_PARTY_INVITE:
        {
            Playerbots::PacketParse::PartyInvitePayload parsed;
            if (!Playerbots::PacketParse::TryReadPartyInvite(*signal.Packet, parsed))
            {
                // Layer 1 failed — ERROR already logged; keep signal for poll/signal fallback.
                return;
            }

            // Layer 2: cross-check InviterGUID vs live group invite leader.
            Player* bot = GetBot();
            Group* invite = bot ? bot->GetGroupInvite() : nullptr;
            ObjectGuid const liveLeader = invite ? invite->GetLeaderGUID() : ObjectGuid::Empty;
            if (liveLeader.IsEmpty() || liveLeader != parsed.InviterGUID)
            {
                TC_LOG_ERROR("playerbots.packet",
                    "BotPacketParse SMSG_PARTY_INVITE Layer-2 mismatch bot={} parsedInviter={} liveLeader={} verifiedBuild={}",
                    bot ? bot->GetName() : "?",
                    parsed.InviterGUID.ToString(),
                    liveLeader.ToString(),
                    Playerbots::PacketParse::VERIFIED_BUILD);
                // Prefer live state for the accept action; signal still fires.
                return;
            }

            if (Playerbots::GetLogLevel() >= 1)
                TC_LOG_DEBUG("playerbots.packet",
                    "BotPacketParse SMSG_PARTY_INVITE ok bot={} inviter={} name={}",
                    bot ? bot->GetName() : "?",
                    parsed.InviterGUID.ToString(),
                    parsed.InviterName);
            break;
        }
        case SMSG_GUILD_INVITE:
        {
            Playerbots::PacketParse::GuildInvitePayload parsed;
            if (!Playerbots::PacketParse::TryReadGuildInvite(*signal.Packet, parsed))
                return;

            // Layer 2: pending guild id vs parsed GuildGUID counter (GetGuildIdInvited dual).
            Player* bot = GetBot();
            ObjectGuid::LowType const liveInvited = bot ? bot->GetGuildIdInvited() : 0;
            ObjectGuid::LowType const parsedGuildId = parsed.GuildGUID.GetCounter();
            if (!liveInvited || liveInvited != parsedGuildId)
            {
                TC_LOG_ERROR("playerbots.packet",
                    "BotPacketParse SMSG_GUILD_INVITE Layer-2 mismatch bot={} parsedGuild={} liveInvited={} verifiedBuild={}",
                    bot ? bot->GetName() : "?",
                    parsed.GuildGUID.ToString(),
                    liveInvited,
                    Playerbots::PacketParse::VERIFIED_BUILD);
                return;
            }

            if (Playerbots::GetLogLevel() >= 1)
                TC_LOG_DEBUG("playerbots.packet",
                    "BotPacketParse SMSG_GUILD_INVITE ok bot={} guild={} name={} inviter={}",
                    bot ? bot->GetName() : "?",
                    parsed.GuildGUID.ToString(),
                    parsed.GuildName,
                    parsed.InviterName);
            break;
        }
        case SMSG_RESURRECT_REQUEST:
        {
            Playerbots::PacketParse::ResurrectRequestPayload parsed;
            if (!Playerbots::PacketParse::TryReadResurrectRequest(*signal.Packet, parsed))
                return;

            // Layer 2: pending resurrection dual vs parsed offerer GUID.
            Player* bot = GetBot();
            if (!bot || !bot->IsResurrectRequested() ||
                !bot->IsResurrectRequestedBy(parsed.ResurrectOffererGUID))
            {
                TC_LOG_ERROR("playerbots.packet",
                    "BotPacketParse SMSG_RESURRECT_REQUEST Layer-2 mismatch bot={} parsedOfferer={} liveRequested={} verifiedBuild={}",
                    bot ? bot->GetName() : "?",
                    parsed.ResurrectOffererGUID.ToString(),
                    bot && bot->IsResurrectRequested() ? bot->GetResurrectRequesterGUID().ToString() : "none",
                    Playerbots::PacketParse::VERIFIED_BUILD);
                return;
            }

            if (Playerbots::GetLogLevel() >= 1)
                TC_LOG_DEBUG("playerbots.packet",
                    "BotPacketParse SMSG_RESURRECT_REQUEST ok bot={} offerer={} name={} spell={}",
                    bot->GetName(),
                    parsed.ResurrectOffererGUID.ToString(),
                    parsed.Name,
                    parsed.SpellID);
            break;
        }
        case SMSG_PETITION_SHOW_SIGNATURES:
        {
            Playerbots::PacketParse::PetitionShowSignaturesPayload parsed;
            if (!Playerbots::PacketParse::TryReadPetitionShowSignatures(*signal.Packet, parsed))
                return;

            // Layer 2: PetitionMgr item/owner dual (+ PetitionID == item counter).
            Petition const* petition = sPetitionMgr->GetPetition(parsed.Item);
            bool const layer2Ok = petition
                && petition->OwnerGuid == parsed.Owner
                && int32(parsed.Item.GetCounter()) == parsed.PetitionID;
            if (!layer2Ok)
            {
                TC_LOG_ERROR("playerbots.packet",
                    "BotPacketParse SMSG_PETITION_SHOW_SIGNATURES Layer-2 mismatch bot={} parsedItem={} parsedOwner={} petitionId={} liveOwner={} verifiedBuild={}",
                    GetBot() ? GetBot()->GetName() : "?",
                    parsed.Item.ToString(),
                    parsed.Owner.ToString(),
                    parsed.PetitionID,
                    petition ? petition->OwnerGuid.ToString() : "none",
                    Playerbots::PacketParse::VERIFIED_BUILD);
                return;
            }

            SetPendingPetitionOffer(parsed.Item);

            if (Playerbots::GetLogLevel() >= 1)
                TC_LOG_DEBUG("playerbots.packet",
                    "BotPacketParse SMSG_PETITION_SHOW_SIGNATURES ok bot={} item={} owner={} petitionId={} signatures={}",
                    GetBot() ? GetBot()->GetName() : "?",
                    parsed.Item.ToString(),
                    parsed.Owner.ToString(),
                    parsed.PetitionID,
                    parsed.Signatures.size());
            break;
        }
        case SMSG_BUY_FAILED:
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
                    GetBot() ? GetBot()->GetName() : "?",
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
                        GetBot() ? GetBot()->GetName() : "?",
                        int32(parsed.Reason),
                        parsed.Muid);
                return;
            }

            Player* bot = GetBot();
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
            break;
        }
        case SMSG_GROUP_NEW_LEADER:
        {
            // Cleared unless Layer 2 OK — do not fire "reset botAI" on mis-parse / mismatch.
            signal.Name.clear();

            Playerbots::PacketParse::GroupNewLeaderPayload parsed;
            if (!Playerbots::PacketParse::TryReadGroupNewLeader(*signal.Packet, parsed))
                return;

            Player* bot = GetBot();
            Group* group = bot ? bot->GetGroup() : nullptr;
            char const* liveLeaderName = group ? group->GetLeaderName() : nullptr;
            bool const nameOk = group && liveLeaderName && !parsed.Name.empty() &&
                parsed.Name == liveLeaderName;
            bool const categoryOk = group &&
                parsed.PartyIndex == int8(group->GetGroupCategory());
            if (!nameOk || !categoryOk)
            {
                TC_LOG_ERROR("playerbots.packet",
                    "BotPacketParse SMSG_GROUP_NEW_LEADER Layer-2 mismatch bot={} parsedName='{}' parsedPartyIndex={} liveLeader='{}' liveCategory={} verifiedBuild={}",
                    bot ? bot->GetName() : "?",
                    parsed.Name,
                    int32(parsed.PartyIndex),
                    liveLeaderName ? liveLeaderName : "none",
                    group ? int32(group->GetGroupCategory()) : -1,
                    Playerbots::PacketParse::VERIFIED_BUILD);
                return;
            }

            signal.Name = "group set leader";

            if (Playerbots::GetLogLevel() >= 1)
                TC_LOG_DEBUG("playerbots.packet",
                    "BotPacketParse SMSG_GROUP_NEW_LEADER ok bot={} leader='{}' partyIndex={}",
                    bot->GetName(),
                    parsed.Name,
                    int32(parsed.PartyIndex));
            break;
        }
        case SMSG_MOVE_SET_RUN_SPEED:
        {
            // Cleared unless Layer 2 OK — do not fire "check mount state" on mis-parse / mismatch.
            signal.Name.clear();

            Playerbots::PacketParse::MoveSetRunSpeedPayload parsed;
            if (!Playerbots::PacketParse::TryReadMoveSetRunSpeed(*signal.Packet, parsed))
                return;

            Player* bot = GetBot();
            if (!bot || parsed.MoverGUID.IsEmpty())
            {
                TC_LOG_ERROR("playerbots.packet",
                    "BotPacketParse SMSG_MOVE_SET_RUN_SPEED Layer-2 empty mover bot={} verifiedBuild={}",
                    bot ? bot->GetName() : "?",
                    Playerbots::PacketParse::VERIFIED_BUILD);
                return;
            }

            // Self-session force-set only; ignore (no reaction) if somehow not the bot mover.
            if (parsed.MoverGUID != bot->GetGUID())
            {
                if (Playerbots::GetLogLevel() >= 1)
                    TC_LOG_DEBUG("playerbots.packet",
                        "BotPacketParse SMSG_MOVE_SET_RUN_SPEED ignored (not self) bot={} mover={}",
                        bot->GetName(),
                        parsed.MoverGUID.ToString());
                return;
            }

            constexpr float kRunSpeedEpsilon = 0.01f;
            float const liveSpeed = bot->GetSpeed(MOVE_RUN);
            if (std::fabs(parsed.Speed - liveSpeed) > kRunSpeedEpsilon)
            {
                TC_LOG_ERROR("playerbots.packet",
                    "BotPacketParse SMSG_MOVE_SET_RUN_SPEED Layer-2 mismatch bot={} parsedSpeed={} liveSpeed={} verifiedBuild={}",
                    bot->GetName(),
                    parsed.Speed,
                    liveSpeed,
                    Playerbots::PacketParse::VERIFIED_BUILD);
                return;
            }

            signal.Name = "check mount state";

            if (Playerbots::GetLogLevel() >= 1)
                TC_LOG_DEBUG("playerbots.packet",
                    "BotPacketParse SMSG_MOVE_SET_RUN_SPEED ok bot={} speed={} seq={}",
                    bot->GetName(),
                    parsed.Speed,
                    parsed.SequenceIndex);
            break;
        }
        case SMSG_INVENTORY_CHANGE_FAILURE:
        {
            // Cleared unless Layer 2 OK + V1 tell map hit — AC second name
            // ("inventory change failure") is the same reaction; no second FollowMaster trigger.
            signal.Name.clear();
            ClearPendingCannotEquipTell();

            Playerbots::PacketParse::InventoryChangeFailurePayload parsed;
            if (!Playerbots::PacketParse::TryReadInventoryChangeFailure(*signal.Packet, parsed))
                return;

            if (parsed.BagResult == EQUIP_ERR_OK)
            {
                if (Playerbots::GetLogLevel() >= 1)
                    TC_LOG_DEBUG("playerbots.packet",
                        "BotPacketParse SMSG_INVENTORY_CHANGE_FAILURE ignored (EQUIP_ERR_OK) bot={}",
                        GetBot() ? GetBot()->GetName() : "?");
                return;
            }

            if (!IsKnownInventoryResult(int32(parsed.BagResult)))
            {
                TC_LOG_ERROR("playerbots.packet",
                    "BotPacketParse SMSG_INVENTORY_CHANGE_FAILURE Layer-2 unknown BagResult bot={} bagResult={} verifiedBuild={}",
                    GetBot() ? GetBot()->GetName() : "?",
                    int32(parsed.BagResult),
                    Playerbots::PacketParse::VERIFIED_BUILD);
                return;
            }

            Player* bot = GetBot();
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

            SetPendingCannotEquipTell(tell);
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
            break;
        }
        case SMSG_TRADE_STATUS:
        {
            // Cleared unless Layer 2 OK + V1 Status (PROPOSED / ACCEPTED). Other statuses
            // parse-ok DEBUG only (COMPLETE/CANCELLED may race TradeData clear — OK).
            signal.Name.clear();
            ClearPendingTradeStatus();

            Playerbots::PacketParse::TradeStatusPayload parsed;
            if (!Playerbots::PacketParse::TryReadTradeStatus(*signal.Packet, parsed))
                return;

            if (!Playerbots::PacketParse::IsKnownTradeStatus(parsed.Status))
            {
                TC_LOG_ERROR("playerbots.packet",
                    "BotPacketParse SMSG_TRADE_STATUS Layer-2 unknown Status bot={} status={} verifiedBuild={}",
                    GetBot() ? GetBot()->GetName() : "?",
                    uint32(parsed.Status),
                    Playerbots::PacketParse::VERIFIED_BUILD);
                return;
            }

            Player* bot = GetBot();
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

                SetPendingTradeStatus(TRADE_STATUS_PROPOSED);
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

                SetPendingTradeStatus(TRADE_STATUS_ACCEPTED);
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
            break;
        }
        default:
            break;
    }
}

void BotPlayerbotAI::UpdateAIInternal(uint32 diff)
{
    std::vector<QueuedSignal> drained;
    {
        std::lock_guard<std::mutex> lock(_signalMutex);
        drained.swap(_signalQueue);
    }

    for (QueuedSignal& queued : drained)
    {
        ProcessPayloadOnTick(queued);

        // Buy-failed clears Name on non-V1 / Layer fail; placeholder "buy failed" must never
        // reach pending (PayloadParse off, or rewrite skipped).
        if (queued.Name.empty() || queued.Name == "buy failed")
            continue;

        if (_pendingSignals.size() >= BOT_SIGNAL_PENDING_MAX)
        {
            if (Playerbots::GetLogLevel() >= 1)
                TC_LOG_DEBUG("playerbots", "BotPlayerbotAI pending signal overflow (dropped oldest) for bot {}",
                    GetBot() ? GetBot()->GetName() : "?");

            _pendingSignals.erase(_pendingSignals.begin());
        }

        _pendingSignals.push_back({ std::move(queued.Name), BOT_SIGNAL_TTL_TICKS });
    }

    // Pending-signal TTL must only burn on ticks where ProcessTriggers ran. Default GCD (500ms)
    // is longer than TTL (3 × ReactDelay ~100ms): signal-only reactions (e.g. "group set leader")
    // otherwise expire while follow keeps the engine on cooldown — poll duals masked that bug.
    bool triggersProcessed = false;
    if (_engine)
        _engine->DoNextAction(diff, triggersProcessed);

    if (!triggersProcessed)
        return;

    for (auto itr = _pendingSignals.begin(); itr != _pendingSignals.end();)
    {
        if (itr->TicksRemaining > 1)
        {
            --itr->TicksRemaining;
            ++itr;
            continue;
        }

        if (Playerbots::GetLogLevel() >= 1)
            TC_LOG_DEBUG("playerbots", "BotPlayerbotAI expired signal '{}' for bot {}",
                itr->Name, GetBot() ? GetBot()->GetName() : "?");

        itr = _pendingSignals.erase(itr);
    }
}
