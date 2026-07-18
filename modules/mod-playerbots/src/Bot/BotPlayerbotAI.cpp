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
#include "Bot/Packet/BotPartyCommandResultPacket.h"
#include "Bot/Packet/BotLevelUpInfoPacket.h"
#include "Bot/Packet/BotLogXPGainPacket.h"
#include "Bot/Packet/BotCastFailedPacket.h"
#include "Bot/Packet/BotEmotePacket.h"
#include "Bot/Packet/BotSTextEmotePacket.h"
#include "Bot/Packet/BotDuelRequestedPacket.h"
#include "Bot/Packet/BotLFGProposalUpdatePacket.h"
#include "Bot/Packet/BotLFGRoleCheckUpdatePacket.h"
#include "Bot/Packet/BotPartyInvitePacket.h"
#include "Bot/Packet/BotPacketParse.h"
#include "Bot/Packet/BotResurrectRequestPacket.h"
#include "Bot/Packet/BotItemPushResultPacket.h"
#include "Bot/Packet/BotLootResponsePacket.h"
#include "Bot/Packet/BotLootRollWonPacket.h"
#include "Bot/Packet/BotStartLootRollPacket.h"
#include "Bot/Packet/BotTradeStatusPacket.h"
#include "Bot/Packet/BotTradeUpdatedPacket.h"
#include "Loot.h"
#include "TradeData.h"
#include "Creature.h"
#include "DB2Stores.h"
#include "DBCEnums.h"
#include "Engine.h"
#include "Group.h"
#include "Item.h"
#include "ItemDefines.h"
#include "LFG.h"
#include "LFGMgr.h"
#include "Log.h"
#include "Map.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "Opcodes.h"
#include "Unit.h"
#include "PetitionMgr.h"
#include "Player.h"
#include "PlayerbotsConfig.h"
#include "Random.h"
#include "RandomPlayerbotMgr.h"
#include "SharedDefines.h"
#include "SpellMgr.h"
#include "WorldPacket.h"
#include "WorldSession.h"
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
        // AC SMSG_TRADE_STATUS_EXTENDED → Midnight SMSG_TRADE_UPDATED; Cleared unless Layer 2 OK
        // (and when PayloadParse is off — no poll dual for trade item/gold list).
        { SMSG_TRADE_UPDATED, { "trade status extended", true } },
        // Cleared unless Layer 2 OK and Acquired (V1 store). Enable=0 / !Acquired: no poll dual.
        { SMSG_LOOT_RESPONSE, { "loot response", true } },
        // Cleared unless Layer 2 OK on self inventory dual. Enable=0 / party broadcast: no poll dual.
        { SMSG_ITEM_PUSH_RESULT, { "item push result", true } },
        // Cleared unless Layer 2 OK (RollType/Winner + soft group/GetLootRoll). Enable=0: no poll dual.
        { SMSG_LOOT_ROLL_WON, { "loot roll won", true } },
        // Cleared unless Layer 2 OK (GROUP/NBG Method + live GetLootRoll). Enable=0: no poll dual.
        { SMSG_START_LOOT_ROLL, { "master loot roll", true } },
        // Cleared unless Layer 2 OK (known Command/Result). Enable=0: no poll dual.
        { SMSG_PARTY_COMMAND_RESULT, { "party command", true } },
        // Cleared unless Layer 2 OK (Level bounds + GetLevel dual). Enable=0: no poll dual.
        { SMSG_LEVEL_UP_INFO, { "levelup", true } },
        // AC SMSG_LOG_XPGAIN → Midnight SMSG_LOG_XP_GAIN; Cleared unless Layer 2 OK
        // (Reason + Amount/Original). Enable=0: no poll dual. Signal+action both "xpgain".
        { SMSG_LOG_XP_GAIN, { "xpgain", true } },
        // Cleared unless Layer 2 OK (Reason range + known SpellID). Enable=0: no poll dual.
        // Signal "cast failed" → action "tell cast failed" (AC names; AC strategy TriggerNode gap).
        { SMSG_CAST_FAILED, { "cast failed", true } },
        // Cleared unless Layer 2 OK (source sanity + soft player/NPC/self). Enable=0: no poll dual.
        // Signal+action both "receive text emote" / "receive emote" (no EmoteStrategy / ReceiveEmote matrix).
        { SMSG_TEXT_EMOTE, { "receive text emote", true } },
        { SMSG_EMOTE, { "receive emote", true } },
        // Cleared unless Layer 2 OK + master challenger (V1 accept). Enable=0: poll twin may still accept.
        // Soft-skip when bot is Initiator (self-observation). Signal "duel requested" → "accept duel".
        { SMSG_DUEL_REQUESTED, { "duel requested", true } },
        // Cleared unless Layer 2 OK (sLFGMgr ROLECHECK / PROPOSAL). Enable=0: proposal poll twin may accept if stash present.
        { SMSG_LFG_ROLE_CHECK_UPDATE, { "lfg role check", true } },
        { SMSG_LFG_PROPOSAL_UPDATE, { "lfg proposal", true } },
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
            signal.Name == "cannot equip" || signal.Name == "trade status" ||
            signal.Name == "trade status extended" || signal.Name == "loot response" ||
            signal.Name == "item push result" || signal.Name == "loot roll won" ||
            signal.Name == "master loot roll" || signal.Name == "party command" ||
            signal.Name == "levelup" || signal.Name == "xpgain" ||
            signal.Name == "cast failed" || signal.Name == "receive text emote" ||
            signal.Name == "receive emote" || signal.Name == "duel requested" ||
            signal.Name == "lfg role check" || signal.Name == "lfg proposal")
        {
            if (signal.Name == "cannot equip")
                ClearPendingCannotEquipTell();
            if (signal.Name == "trade status")
                ClearPendingTradeStatus();
            if (signal.Name == "trade status extended")
                ClearPendingTradeUpdatedLockedTell();
            if (signal.Name == "loot response")
                ClearPendingLootStore();
            if (signal.Name == "item push result")
                ClearPendingItemPush();
            if (signal.Name == "loot roll won")
                ClearPendingLootRollWon();
            if (signal.Name == "master loot roll")
                ClearPendingMasterLootRoll();
            if (signal.Name == "party command")
                ClearPendingPartyCommand();
            if (signal.Name == "levelup")
                ClearPendingLevelUp();
            if (signal.Name == "xpgain")
                ClearPendingXpGain();
            if (signal.Name == "cast failed")
                ClearPendingCastFailed();
            if (signal.Name == "receive text emote" || signal.Name == "receive emote")
                ClearPendingReceiveEmote();
            if (signal.Name == "duel requested")
                ClearPendingDuelArbiter();
            if (signal.Name == "lfg proposal")
                ClearPendingLfgProposal();
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
        case SMSG_TRADE_UPDATED:
        {
            // Cleared unless Layer 2 OK. Enable=0 / mismatch: no poll dual for item/gold list.
            signal.Name.clear();
            ClearPendingTradeUpdatedLockedTell();

            Playerbots::PacketParse::TradeUpdatedPayload parsed;
            if (!Playerbots::PacketParse::TryReadTradeUpdated(*signal.Packet, parsed))
                return;

            Player* bot = GetBot();
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

            Player* master = GetMaster();
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
                SetPendingTradeUpdatedLockedTell(true);

            if (Playerbots::GetLogLevel() >= 1)
                TC_LOG_DEBUG("playerbots.packet",
                    "BotPacketParse SMSG_TRADE_UPDATED ok bot={} whichPlayer={} gold={} state={} items={} lockedNonTradedTell={}",
                    bot->GetName(),
                    uint32(parsed.WhichPlayer),
                    parsed.Gold,
                    parsed.CurrentStateIndex,
                    parsed.Items.size(),
                    GetPendingTradeUpdatedLockedTell() ? "yes" : "no");
            break;
        }
        case SMSG_LOOT_RESPONSE:
        {
            // Cleared unless Layer 2 OK + Acquired. Enable=0 / !Acquired / mismatch: no poll dual.
            signal.Name.clear();
            ClearPendingLootStore();

            Playerbots::PacketParse::LootResponsePayload parsed;
            if (!Playerbots::PacketParse::TryReadLootResponse(*signal.Packet, parsed))
                return;

            if (!Playerbots::PacketParse::IsKnownLootError(parsed.FailureReason))
            {
                TC_LOG_ERROR("playerbots.packet",
                    "BotPacketParse SMSG_LOOT_RESPONSE Layer-1 unknown FailureReason bot={} reason={} verifiedBuild={}",
                    GetBot() ? GetBot()->GetName() : "?",
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
                        GetBot() ? GetBot()->GetName() : "?",
                        parsed.Owner.ToString(),
                        uint32(parsed.FailureReason));
                return;
            }

            Player* bot = GetBot();
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

            PendingLootStore stash;
            stash.Owner = parsed.Owner;
            stash.LootObj = parsed.LootObj;
            stash.Coins = parsed.Coins;
            stash.Items.reserve(parsed.Items.size());
            for (WorldPackets::Loot::LootItemData const& item : parsed.Items)
            {
                PendingLootStore::ItemSlot slot;
                slot.LootListID = item.LootListID;
                slot.ItemID = item.Loot.ItemID;
                slot.UIType = item.UIType;
                stash.Items.push_back(slot);
            }

            SetPendingLootStore(std::move(stash));
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
            break;
        }
        case SMSG_ITEM_PUSH_RESULT:
        {
            // Cleared unless Layer 2 OK on self. Enable=0 / party broadcast / mismatch: no poll dual.
            signal.Name.clear();
            ClearPendingItemPush();

            Playerbots::PacketParse::ItemPushResultPayload parsed;
            if (!Playerbots::PacketParse::TryReadItemPushResult(*signal.Packet, parsed))
                return;

            Player* bot = GetBot();
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

            PendingItemPush stash;
            stash.ItemID = parsed.Item.ItemID;
            stash.ProxyItemID = parsed.ProxyItemID;
            stash.Quantity = parsed.Quantity;
            stash.QuantityInInventory = parsed.QuantityInInventory;
            SetPendingItemPush(std::move(stash));
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
            break;
        }
        case SMSG_LOOT_ROLL_WON:
        {
            // Cleared unless Layer 2 OK. Enable=0 / mismatch: no poll dual (roll usually already gone).
            signal.Name.clear();
            ClearPendingLootRollWon();

            Playerbots::PacketParse::LootRollWonPayload parsed;
            if (!Playerbots::PacketParse::TryReadLootRollWon(*signal.Packet, parsed))
                return;

            if (!Playerbots::PacketParse::IsKnownLootRollWonRollType(parsed.RollType))
            {
                TC_LOG_ERROR("playerbots.packet",
                    "BotPacketParse SMSG_LOOT_ROLL_WON Layer-2 unknown RollType bot={} rollType={} verifiedBuild={}",
                    GetBot() ? GetBot()->GetName() : "?",
                    uint32(parsed.RollType),
                    Playerbots::PacketParse::VERIFIED_BUILD);
                return;
            }

            if (parsed.Winner.IsEmpty())
            {
                TC_LOG_ERROR("playerbots.packet",
                    "BotPacketParse SMSG_LOOT_ROLL_WON Layer-2 empty Winner bot={} lootObj={} verifiedBuild={}",
                    GetBot() ? GetBot()->GetName() : "?",
                    parsed.LootObj.ToString(),
                    Playerbots::PacketParse::VERIFIED_BUILD);
                return;
            }

            Player* bot = GetBot();
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
            PendingLootRollWon stash;
            stash.Winner = parsed.Winner;
            stash.ItemID = parsed.Item.Loot.ItemID;
            SetPendingLootRollWon(std::move(stash));
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
            break;
        }
        case SMSG_START_LOOT_ROLL:
        {
            // Cleared unless Layer 2 OK. Enable=0 / mismatch: no poll dual.
            signal.Name.clear();
            ClearPendingMasterLootRoll();

            Playerbots::PacketParse::StartLootRollPayload parsed;
            if (!Playerbots::PacketParse::TryReadStartLootRoll(*signal.Packet, parsed))
                return;

            if (!Playerbots::PacketParse::IsStartLootRollMethod(parsed.Method))
            {
                TC_LOG_ERROR("playerbots.packet",
                    "BotPacketParse SMSG_START_LOOT_ROLL Layer-2 unexpected Method bot={} method={} verifiedBuild={}",
                    GetBot() ? GetBot()->GetName() : "?",
                    uint32(parsed.Method),
                    Playerbots::PacketParse::VERIFIED_BUILD);
                return;
            }

            Player* bot = GetBot();
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
            PendingMasterLootRoll stash;
            stash.LootObj = parsed.LootObj;
            stash.LootListID = parsed.Item.LootListID;
            stash.ItemID = parsed.Item.Loot.ItemID;
            SetPendingMasterLootRoll(std::move(stash));
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
            break;
        }
        case SMSG_PARTY_COMMAND_RESULT:
        {
            // Cleared unless Layer 2 OK. Enable=0 / mismatch: no poll dual.
            signal.Name.clear();
            ClearPendingPartyCommand();

            Playerbots::PacketParse::PartyCommandResultPayload parsed;
            if (!Playerbots::PacketParse::TryReadPartyCommandResult(*signal.Packet, parsed))
                return;

            if (!Playerbots::PacketParse::IsKnownPartyOperation(parsed.Command))
            {
                TC_LOG_ERROR("playerbots.packet",
                    "BotPacketParse SMSG_PARTY_COMMAND_RESULT Layer-2 unknown Command bot={} command={} verifiedBuild={}",
                    GetBot() ? GetBot()->GetName() : "?",
                    uint32(parsed.Command),
                    Playerbots::PacketParse::VERIFIED_BUILD);
                return;
            }

            // Documented PartyResult max is ERR_PARTY_LFG_TELEPORT_IN_COMBAT (30).
            // Outside that → ERROR. Gaps inside the range (10/11) → soft parse-ok, no reaction.
            if (parsed.Result > uint8(ERR_PARTY_LFG_TELEPORT_IN_COMBAT))
            {
                TC_LOG_ERROR("playerbots.packet",
                    "BotPacketParse SMSG_PARTY_COMMAND_RESULT Layer-2 Result out of documented range bot={} result={} verifiedBuild={}",
                    GetBot() ? GetBot()->GetName() : "?",
                    uint32(parsed.Result),
                    Playerbots::PacketParse::VERIFIED_BUILD);
                return;
            }

            if (!Playerbots::PacketParse::IsKnownPartyResult(parsed.Result))
            {
                if (Playerbots::GetLogLevel() >= 1)
                    TC_LOG_DEBUG("playerbots.packet",
                        "BotPacketParse SMSG_PARTY_COMMAND_RESULT ok (unknown-but-plausible Result, no reaction) bot={} command={} result={} name='{}'",
                        GetBot() ? GetBot()->GetName() : "?",
                        uint32(parsed.Command),
                        uint32(parsed.Result),
                        parsed.Name);
                return;
            }

            Player* bot = GetBot();
            Player* master = GetMaster();
            // Soft leave dual: do not hard-require GetGroup() after LEAVE OK (group may be gone).
            // Prefer Name matches bot or master when present; mismatch is not a Layer-2 ERROR.
            if (parsed.Command == uint8(PARTY_OP_LEAVE) &&
                parsed.Result == uint8(ERR_PARTY_RESULT_OK) &&
                !parsed.Name.empty() &&
                bot &&
                parsed.Name != bot->GetName() &&
                (!master || parsed.Name != master->GetName()) &&
                Playerbots::GetLogLevel() >= 1)
            {
                TC_LOG_DEBUG("playerbots.packet",
                    "BotPacketParse SMSG_PARTY_COMMAND_RESULT LEAVE OK soft Name dual bot={} name='{}' master='{}'",
                    bot->GetName(),
                    parsed.Name,
                    master ? master->GetName() : "none");
            }

            PendingPartyCommand stash;
            stash.Command = parsed.Command;
            stash.Result = parsed.Result;
            stash.Name = parsed.Name;
            SetPendingPartyCommand(std::move(stash));
            signal.Name = "party command";

            if (Playerbots::GetLogLevel() >= 1)
                TC_LOG_DEBUG("playerbots.packet",
                    "BotPacketParse SMSG_PARTY_COMMAND_RESULT ok bot={} command={} result={} name='{}' resultData={} resultGuid={}",
                    bot ? bot->GetName() : "?",
                    uint32(parsed.Command),
                    uint32(parsed.Result),
                    parsed.Name,
                    parsed.ResultData,
                    parsed.ResultGUID.ToString());
            break;
        }
        case SMSG_LEVEL_UP_INFO:
        {
            // Cleared unless Layer 2 OK. Enable=0 / mismatch: no poll dual.
            signal.Name.clear();
            ClearPendingLevelUp();

            Playerbots::PacketParse::LevelUpInfoPayload parsed;
            if (!Playerbots::PacketParse::TryReadLevelUpInfo(*signal.Packet, parsed))
                return;

            // Sane level bounds: reject 0 / negative / beyond server-side strong max.
            if (parsed.Level < 1 || parsed.Level > int32(STRONG_MAX_LEVEL))
            {
                TC_LOG_ERROR("playerbots.packet",
                    "BotPacketParse SMSG_LEVEL_UP_INFO Layer-2 Level out of bounds bot={} level={} verifiedBuild={}",
                    GetBot() ? GetBot()->GetName() : "?",
                    parsed.Level,
                    Playerbots::PacketParse::VERIFIED_BUILD);
                return;
            }

            Player* bot = GetBot();
            if (!bot || parsed.Level != int32(bot->GetLevel()))
            {
                TC_LOG_ERROR("playerbots.packet",
                    "BotPacketParse SMSG_LEVEL_UP_INFO Layer-2 Level mismatch bot={} parsed={} live={} verifiedBuild={}",
                    bot ? bot->GetName() : "?",
                    parsed.Level,
                    bot ? int32(bot->GetLevel()) : -1,
                    Playerbots::PacketParse::VERIFIED_BUILD);
                return;
            }

            // Soft: negative talent slot deltas can occur on delevel; log only.
            if ((parsed.NumNewTalents < 0 || parsed.NumNewPvpTalentSlots < 0) &&
                Playerbots::GetLogLevel() >= 1)
            {
                TC_LOG_DEBUG("playerbots.packet",
                    "BotPacketParse SMSG_LEVEL_UP_INFO soft negative talent deltas bot={} level={} talents={} pvpSlots={}",
                    bot->GetName(),
                    parsed.Level,
                    parsed.NumNewTalents,
                    parsed.NumNewPvpTalentSlots);
            }

            PendingLevelUp stash;
            stash.Level = parsed.Level;
            SetPendingLevelUp(std::move(stash));
            signal.Name = "levelup";

            if (Playerbots::GetLogLevel() >= 1)
                TC_LOG_DEBUG("playerbots.packet",
                    "BotPacketParse SMSG_LEVEL_UP_INFO ok bot={} level={} healthDelta={} talents={} pvpSlots={}",
                    bot->GetName(),
                    parsed.Level,
                    parsed.HealthDelta,
                    parsed.NumNewTalents,
                    parsed.NumNewPvpTalentSlots);
            break;
        }
        case SMSG_LOG_XP_GAIN:
        {
            // Cleared unless Layer 2 OK. Enable=0 / mismatch: no poll dual.
            // Do not hard-dual Amount vs GetXP() after apply (packet sent before SetXP).
            signal.Name.clear();
            ClearPendingXpGain();

            Playerbots::PacketParse::LogXPGainPayload parsed;
            if (!Playerbots::PacketParse::TryReadLogXPGain(*signal.Packet, parsed))
                return;

            if (parsed.Reason != LOG_XP_REASON_KILL && parsed.Reason != LOG_XP_REASON_NO_KILL)
            {
                TC_LOG_ERROR("playerbots.packet",
                    "BotPacketParse SMSG_LOG_XP_GAIN Layer-2 unknown Reason bot={} reason={} verifiedBuild={}",
                    GetBot() ? GetBot()->GetName() : "?",
                    uint32(parsed.Reason),
                    Playerbots::PacketParse::VERIFIED_BUILD);
                return;
            }

            if (parsed.Amount <= 0 || parsed.Original < 0 || parsed.Original < parsed.Amount)
            {
                TC_LOG_ERROR("playerbots.packet",
                    "BotPacketParse SMSG_LOG_XP_GAIN Layer-2 Amount/Original invalid bot={} amount={} original={} verifiedBuild={}",
                    GetBot() ? GetBot()->GetName() : "?",
                    parsed.Amount,
                    parsed.Original,
                    Playerbots::PacketParse::VERIFIED_BUILD);
                return;
            }

            if (!std::isfinite(parsed.GroupBonus))
            {
                TC_LOG_ERROR("playerbots.packet",
                    "BotPacketParse SMSG_LOG_XP_GAIN Layer-2 GroupBonus not finite bot={} groupBonus={} verifiedBuild={}",
                    GetBot() ? GetBot()->GetName() : "?",
                    parsed.GroupBonus,
                    Playerbots::PacketParse::VERIFIED_BUILD);
                return;
            }

            Player* bot = GetBot();
            if (std::fabs(parsed.GroupBonus) > 100.0f && Playerbots::GetLogLevel() >= 1)
            {
                TC_LOG_DEBUG("playerbots.packet",
                    "BotPacketParse SMSG_LOG_XP_GAIN soft absurd GroupBonus bot={} groupBonus={}",
                    bot ? bot->GetName() : "?",
                    parsed.GroupBonus);
            }

            // Soft Victim: KILL prefers non-empty map-local unit; NO_KILL empty is expected.
            // Map-local only — do not cold-query remote maps.
            if (parsed.Reason == LOG_XP_REASON_KILL)
            {
                if (parsed.Victim.IsEmpty())
                {
                    if (Playerbots::GetLogLevel() >= 1)
                        TC_LOG_DEBUG("playerbots.packet",
                            "BotPacketParse SMSG_LOG_XP_GAIN soft empty Victim on KILL bot={}",
                            bot ? bot->GetName() : "?");
                }
                else
                {
                    Map* map = bot ? bot->GetMap() : nullptr;
                    Creature* victim = map ? map->GetCreature(parsed.Victim) : nullptr;
                    if (!victim && Playerbots::GetLogLevel() >= 1)
                        TC_LOG_DEBUG("playerbots.packet",
                            "BotPacketParse SMSG_LOG_XP_GAIN soft Victim not map-local bot={} victim={}",
                            bot ? bot->GetName() : "?",
                            parsed.Victim.ToString());
                }
            }

            PendingXpGain stash;
            stash.Amount = parsed.Amount;
            SetPendingXpGain(std::move(stash));
            signal.Name = "xpgain";

            if (Playerbots::GetLogLevel() >= 1)
                TC_LOG_DEBUG("playerbots.packet",
                    "BotPacketParse SMSG_LOG_XP_GAIN ok bot={} amount={} original={} reason={} victim={} groupBonus={}",
                    bot ? bot->GetName() : "?",
                    parsed.Amount,
                    parsed.Original,
                    uint32(parsed.Reason),
                    parsed.Victim.ToString(),
                    parsed.GroupBonus);
            break;
        }
        case SMSG_CAST_FAILED:
        {
            // Cleared unless Layer 2 OK. Enable=0: no poll dual.
            // Payload-only — do not invent GetCurrentSpell / pending-cast hard dual.
            signal.Name.clear();
            ClearPendingCastFailed();

            Playerbots::PacketParse::CastFailedPayload parsed;
            if (!Playerbots::PacketParse::TryReadCastFailed(*signal.Packet, parsed))
                return;

            if (parsed.Reason == SPELL_CAST_OK || parsed.Reason == SPELL_FAILED_SUCCESS)
            {
                TC_LOG_ERROR("playerbots.packet",
                    "BotPacketParse SMSG_CAST_FAILED Layer-2 success Reason on fail SMSG bot={} reason={} verifiedBuild={}",
                    GetBot() ? GetBot()->GetName() : "?",
                    parsed.Reason,
                    Playerbots::PacketParse::VERIFIED_BUILD);
                return;
            }

            if (parsed.Reason < 0 || parsed.Reason > SPELL_FAILED_UNKNOWN)
            {
                TC_LOG_ERROR("playerbots.packet",
                    "BotPacketParse SMSG_CAST_FAILED Layer-2 Reason out of range bot={} reason={} verifiedBuild={}",
                    GetBot() ? GetBot()->GetName() : "?",
                    parsed.Reason,
                    Playerbots::PacketParse::VERIFIED_BUILD);
                return;
            }

            if (parsed.SpellID <= 0 || !sSpellMgr->GetSpellInfo(uint32(parsed.SpellID), DIFFICULTY_NONE))
            {
                TC_LOG_ERROR("playerbots.packet",
                    "BotPacketParse SMSG_CAST_FAILED Layer-2 unknown SpellID bot={} spellId={} verifiedBuild={}",
                    GetBot() ? GetBot()->GetName() : "?",
                    parsed.SpellID,
                    Playerbots::PacketParse::VERIFIED_BUILD);
                return;
            }

            Player* bot = GetBot();
            // Soft FailedBy: empty expected for most Reasons; non-empty prefers map-local unit.
            if (!parsed.FailedBy.IsEmpty() && bot)
            {
                Unit* failedBy = ObjectAccessor::GetUnit(*bot, parsed.FailedBy);
                if (!failedBy && Playerbots::GetLogLevel() >= 1)
                    TC_LOG_DEBUG("playerbots.packet",
                        "BotPacketParse SMSG_CAST_FAILED soft FailedBy not map-local bot={} failedBy={}",
                        bot->GetName(),
                        parsed.FailedBy.ToString());
            }

            PendingCastFailed stash;
            stash.SpellID = parsed.SpellID;
            stash.Reason = parsed.Reason;
            SetPendingCastFailed(std::move(stash));
            signal.Name = "cast failed";

            if (Playerbots::GetLogLevel() >= 1)
                TC_LOG_DEBUG("playerbots.packet",
                    "BotPacketParse SMSG_CAST_FAILED ok bot={} spellId={} reason={} castId={} failedArg1={} failedArg2={} failedBy={}",
                    bot ? bot->GetName() : "?",
                    parsed.SpellID,
                    parsed.Reason,
                    parsed.CastID.ToString(),
                    parsed.FailedArg1,
                    parsed.FailedArg2,
                    parsed.FailedBy.ToString());
            break;
        }
        case SMSG_TEXT_EMOTE:
        {
            // Cleared unless Layer 2 OK. Enable=0: no poll dual.
            // Payload-only — soft player/self/EmoteID filters; no invented pending-emote dual.
            signal.Name.clear();
            ClearPendingReceiveEmote();

            Playerbots::PacketParse::STextEmotePayload parsed;
            if (!Playerbots::PacketParse::TryReadSTextEmote(*signal.Packet, parsed))
                return;

            Player* bot = GetBot();
            if (parsed.SourceGUID.IsEmpty())
            {
                if (Playerbots::GetLogLevel() >= 1)
                    TC_LOG_DEBUG("playerbots.packet",
                        "BotPacketParse SMSG_TEXT_EMOTE soft empty SourceGUID bot={}",
                        bot ? bot->GetName() : "?");
                return;
            }

            if (bot && parsed.SourceGUID == bot->GetGUID())
            {
                if (Playerbots::GetLogLevel() >= 1)
                    TC_LOG_DEBUG("playerbots.packet",
                        "BotPacketParse SMSG_TEXT_EMOTE soft self-source skip bot={}",
                        bot->GetName());
                return;
            }

            if (!parsed.SourceGUID.IsPlayer())
            {
                if (Playerbots::GetLogLevel() >= 1)
                    TC_LOG_DEBUG("playerbots.packet",
                        "BotPacketParse SMSG_TEXT_EMOTE soft non-player source skip bot={} source={}",
                        bot ? bot->GetName() : "?",
                        parsed.SourceGUID.ToString());
                return;
            }

            if (parsed.EmoteID == 0)
            {
                if (Playerbots::GetLogLevel() >= 1)
                    TC_LOG_DEBUG("playerbots.packet",
                        "BotPacketParse SMSG_TEXT_EMOTE soft EmoteID=0 skip bot={}",
                        bot ? bot->GetName() : "?");
                return;
            }

            // Soft TargetGUID: empty is common; non-empty prefers map-local (no cold remote query).
            if (!parsed.TargetGUID.IsEmpty() && bot)
            {
                Unit* target = ObjectAccessor::GetUnit(*bot, parsed.TargetGUID);
                if (!target && Playerbots::GetLogLevel() >= 1)
                    TC_LOG_DEBUG("playerbots.packet",
                        "BotPacketParse SMSG_TEXT_EMOTE soft TargetGUID not map-local bot={} target={}",
                        bot->GetName(),
                        parsed.TargetGUID.ToString());
            }

            PendingReceiveEmote stash;
            stash.SourceGUID = parsed.SourceGUID;
            stash.EmoteID = uint32(parsed.EmoteID);
            stash.IsTextEmote = true;
            SetPendingReceiveEmote(std::move(stash));
            signal.Name = "receive text emote";

            if (Playerbots::GetLogLevel() >= 1)
                TC_LOG_DEBUG("playerbots.packet",
                    "BotPacketParse SMSG_TEXT_EMOTE ok bot={} source={} emoteId={} soundIndex={} target={}",
                    bot ? bot->GetName() : "?",
                    parsed.SourceGUID.ToString(),
                    parsed.EmoteID,
                    parsed.SoundIndex,
                    parsed.TargetGUID.ToString());
            break;
        }
        case SMSG_EMOTE:
        {
            // Cleared unless Layer 2 OK. Enable=0: no poll dual.
            // Payload-only — soft player/self/ONESHOT_NONE filters (AC NPC pre-filter intent).
            signal.Name.clear();
            ClearPendingReceiveEmote();

            Playerbots::PacketParse::EmotePayload parsed;
            if (!Playerbots::PacketParse::TryReadEmote(*signal.Packet, parsed))
                return;

            Player* bot = GetBot();
            if (parsed.Guid.IsEmpty())
            {
                if (Playerbots::GetLogLevel() >= 1)
                    TC_LOG_DEBUG("playerbots.packet",
                        "BotPacketParse SMSG_EMOTE soft empty Guid bot={}",
                        bot ? bot->GetName() : "?");
                return;
            }

            if (bot && parsed.Guid == bot->GetGUID())
            {
                if (Playerbots::GetLogLevel() >= 1)
                    TC_LOG_DEBUG("playerbots.packet",
                        "BotPacketParse SMSG_EMOTE soft self-source skip bot={}",
                        bot->GetName());
                return;
            }

            // Match AC HandleBotOutgoingPacket: skip NPC oneshots without Layer-2 ERROR.
            if (!parsed.Guid.IsPlayer())
            {
                if (Playerbots::GetLogLevel() >= 1)
                    TC_LOG_DEBUG("playerbots.packet",
                        "BotPacketParse SMSG_EMOTE soft non-player source skip bot={} source={}",
                        bot ? bot->GetName() : "?",
                        parsed.Guid.ToString());
                return;
            }

            if (parsed.EmoteID == EMOTE_ONESHOT_NONE)
            {
                if (Playerbots::GetLogLevel() >= 1)
                    TC_LOG_DEBUG("playerbots.packet",
                        "BotPacketParse SMSG_EMOTE soft ONESHOT_NONE skip bot={}",
                        bot ? bot->GetName() : "?");
                return;
            }

            PendingReceiveEmote stash;
            stash.SourceGUID = parsed.Guid;
            stash.EmoteID = parsed.EmoteID;
            stash.IsTextEmote = false;
            SetPendingReceiveEmote(std::move(stash));
            signal.Name = "receive emote";

            if (Playerbots::GetLogLevel() >= 1)
                TC_LOG_DEBUG("playerbots.packet",
                    "BotPacketParse SMSG_EMOTE ok bot={} source={} emoteId={} kits={} sequenceVariation={}",
                    bot ? bot->GetName() : "?",
                    parsed.Guid.ToString(),
                    parsed.EmoteID,
                    parsed.SpellVisualKitIDs.size(),
                    parsed.SequenceVariation);
            break;
        }
        case SMSG_DUEL_REQUESTED:
        {
            // Cleared unless Layer 2 OK + master challenger (V1). Enable=0: poll twin may still accept.
            signal.Name.clear();
            ClearPendingDuelArbiter();

            Playerbots::PacketParse::DuelRequestedPayload parsed;
            if (!Playerbots::PacketParse::TryReadDuelRequested(*signal.Packet, parsed))
                return;

            Player* bot = GetBot();
            if (!bot || !bot->duel || bot->duel->State != DUEL_STATE_CHALLENGED)
            {
                TC_LOG_ERROR("playerbots.packet",
                    "BotPacketParse SMSG_DUEL_REQUESTED Layer-2 mismatch bot={} duelState={} verifiedBuild={}",
                    bot ? bot->GetName() : "?",
                    bot && bot->duel ? uint32(bot->duel->State) : 0u,
                    Playerbots::PacketParse::VERIFIED_BUILD);
                return;
            }

            ObjectGuid const liveArbiter = *bot->m_playerData->DuelArbiter;
            if (liveArbiter.IsEmpty() || liveArbiter != parsed.ArbiterGUID)
            {
                TC_LOG_ERROR("playerbots.packet",
                    "BotPacketParse SMSG_DUEL_REQUESTED Layer-2 Arbiter mismatch bot={} parsed={} live={} verifiedBuild={}",
                    bot->GetName(),
                    parsed.ArbiterGUID.ToString(),
                    liveArbiter.ToString(),
                    Playerbots::PacketParse::VERIFIED_BUILD);
                return;
            }

            if (!bot->duel->Initiator)
            {
                TC_LOG_ERROR("playerbots.packet",
                    "BotPacketParse SMSG_DUEL_REQUESTED Layer-2 null Initiator bot={} verifiedBuild={}",
                    bot->GetName(),
                    Playerbots::PacketParse::VERIFIED_BUILD);
                return;
            }

            ObjectGuid const liveInitiator = bot->duel->Initiator->GetGUID();
            if (liveInitiator != parsed.RequestedByGUID)
            {
                TC_LOG_ERROR("playerbots.packet",
                    "BotPacketParse SMSG_DUEL_REQUESTED Layer-2 Initiator mismatch bot={} parsed={} live={} verifiedBuild={}",
                    bot->GetName(),
                    parsed.RequestedByGUID.ToString(),
                    liveInitiator.ToString(),
                    Playerbots::PacketParse::VERIFIED_BUILD);
                return;
            }

            // Soft: bot is the challenger observing its own SMSG — clear without Layer-2 ERROR.
            if (bot->duel->Initiator == bot)
            {
                if (Playerbots::GetLogLevel() >= 1)
                    TC_LOG_DEBUG("playerbots.packet",
                        "BotPacketParse SMSG_DUEL_REQUESTED soft initiator self-skip bot={}",
                        bot->GetName());
                return;
            }

            // Soft: RequestedBy should be a player; map-local when practical (no cold remote query).
            if (!parsed.RequestedByGUID.IsPlayer())
            {
                if (Playerbots::GetLogLevel() >= 1)
                    TC_LOG_DEBUG("playerbots.packet",
                        "BotPacketParse SMSG_DUEL_REQUESTED soft non-player RequestedBy bot={} requestedBy={}",
                        bot->GetName(),
                        parsed.RequestedByGUID.ToString());
            }
            else if (Player* challenger = ObjectAccessor::GetPlayer(*bot, parsed.RequestedByGUID); !challenger)
            {
                if (Playerbots::GetLogLevel() >= 1)
                    TC_LOG_DEBUG("playerbots.packet",
                        "BotPacketParse SMSG_DUEL_REQUESTED soft RequestedBy not map-local bot={} requestedBy={}",
                        bot->GetName(),
                        parsed.RequestedByGUID.ToString());
            }

            // Soft: ToTheDeath unconstrained (false expected from current SpellEffects sender).
            if (parsed.ToTheDeath && Playerbots::GetLogLevel() >= 1)
                TC_LOG_DEBUG("playerbots.packet",
                    "BotPacketParse SMSG_DUEL_REQUESTED soft ToTheDeath=true bot={}",
                    bot->GetName());

            Player* master = GetMaster();
            bool const masterChallenger = master && parsed.RequestedByGUID == master->GetGUID();

            if (Playerbots::GetLogLevel() >= 1)
                TC_LOG_DEBUG("playerbots.packet",
                    "BotPacketParse SMSG_DUEL_REQUESTED ok bot={} arbiter={} requestedBy={} toTheDeath={} masterChallenger={}",
                    bot->GetName(),
                    parsed.ArbiterGUID.ToString(),
                    parsed.RequestedByGUID.ToString(),
                    parsed.ToTheDeath ? "yes" : "no",
                    masterChallenger ? "yes" : "no");

            // V1 FollowMaster: only enqueue accept signal for master challenger.
            if (!masterChallenger)
                return;

            SetPendingDuelArbiter(parsed.ArbiterGUID);
            signal.Name = "duel requested";
            break;
        }
        case SMSG_LFG_ROLE_CHECK_UPDATE:
        {
            // Cleared unless Layer 2 OK (sLFGMgr ROLECHECK). No stash for role check.
            signal.Name.clear();

            Playerbots::PacketParse::LFGRoleCheckUpdatePayload parsed;
            if (!Playerbots::PacketParse::TryReadLFGRoleCheckUpdate(*signal.Packet, parsed))
                return;

            Player* bot = GetBot();
            ObjectGuid const botGuid = bot ? bot->GetGUID() : ObjectGuid::Empty;
            lfg::LfgState const liveState = bot ? sLFGMgr->GetState(botGuid) : lfg::LFG_STATE_NONE;
            if (liveState != lfg::LFG_STATE_ROLECHECK)
            {
                TC_LOG_ERROR("playerbots.packet",
                    "BotPacketParse SMSG_LFG_ROLE_CHECK_UPDATE Layer-2 state mismatch bot={} liveState={} verifiedBuild={}",
                    bot ? bot->GetName() : "?",
                    uint32(liveState),
                    Playerbots::PacketParse::VERIFIED_BUILD);
                return;
            }

            if (parsed.RoleCheckStatus > uint8(lfg::LFG_ROLECHECK_NO_ROLE))
            {
                TC_LOG_ERROR("playerbots.packet",
                    "BotPacketParse SMSG_LFG_ROLE_CHECK_UPDATE Layer-2 bad RoleCheckStatus bot={} status={} verifiedBuild={}",
                    bot->GetName(),
                    uint32(parsed.RoleCheckStatus),
                    Playerbots::PacketParse::VERIFIED_BUILD);
                return;
            }

            // Soft: prefer bot in a group (HandleLfgSetRolesOpcode requires it).
            if (!bot->GetGroup() && Playerbots::GetLogLevel() >= 1)
                TC_LOG_DEBUG("playerbots.packet",
                    "BotPacketParse SMSG_LFG_ROLE_CHECK_UPDATE soft no group bot={}",
                    bot->GetName());

            // Soft: Members non-empty and includes bot Guid.
            bool memberIncludesBot = false;
            for (auto const& member : parsed.Members)
            {
                if (member.Guid == botGuid)
                {
                    memberIncludesBot = true;
                    break;
                }
            }
            if ((parsed.Members.empty() || !memberIncludesBot) && Playerbots::GetLogLevel() >= 1)
                TC_LOG_DEBUG("playerbots.packet",
                    "BotPacketParse SMSG_LFG_ROLE_CHECK_UPDATE soft Members bot={} count={} includesBot={}",
                    bot->GetName(),
                    parsed.Members.size(),
                    memberIncludesBot ? "yes" : "no");

            if (Playerbots::GetLogLevel() >= 1)
                TC_LOG_DEBUG("playerbots.packet",
                    "BotPacketParse SMSG_LFG_ROLE_CHECK_UPDATE ok bot={} status={} members={} joinSlots={} beginning={}",
                    bot->GetName(),
                    uint32(parsed.RoleCheckStatus),
                    parsed.Members.size(),
                    parsed.JoinSlots.size(),
                    parsed.IsBeginning ? "yes" : "no");

            signal.Name = "lfg role check";
            break;
        }
        case SMSG_LFG_PROPOSAL_UPDATE:
        {
            // Cleared unless Layer 2 OK (sLFGMgr PROPOSAL + ProposalID). Stash for accept.
            signal.Name.clear();
            ClearPendingLfgProposal();

            Playerbots::PacketParse::LFGProposalUpdatePayload parsed;
            if (!Playerbots::PacketParse::TryReadLFGProposalUpdate(*signal.Packet, parsed))
                return;

            Player* bot = GetBot();
            ObjectGuid const botGuid = bot ? bot->GetGUID() : ObjectGuid::Empty;
            lfg::LfgState const liveState = bot ? sLFGMgr->GetState(botGuid) : lfg::LFG_STATE_NONE;
            if (liveState != lfg::LFG_STATE_PROPOSAL)
            {
                TC_LOG_ERROR("playerbots.packet",
                    "BotPacketParse SMSG_LFG_PROPOSAL_UPDATE Layer-2 state mismatch bot={} liveState={} verifiedBuild={}",
                    bot ? bot->GetName() : "?",
                    uint32(liveState),
                    Playerbots::PacketParse::VERIFIED_BUILD);
                return;
            }

            if (!parsed.ProposalID)
            {
                TC_LOG_ERROR("playerbots.packet",
                    "BotPacketParse SMSG_LFG_PROPOSAL_UPDATE Layer-2 ProposalID=0 bot={} verifiedBuild={}",
                    bot->GetName(),
                    Playerbots::PacketParse::VERIFIED_BUILD);
                return;
            }

            // Soft: Players contains Me == true.
            bool hasMe = false;
            for (auto const& player : parsed.Players)
            {
                if (player.Me)
                {
                    hasMe = true;
                    break;
                }
            }
            if (!hasMe && Playerbots::GetLogLevel() >= 1)
                TC_LOG_DEBUG("playerbots.packet",
                    "BotPacketParse SMSG_LFG_PROPOSAL_UPDATE soft no Me entry bot={} players={}",
                    bot->GetName(),
                    parsed.Players.size());

            // Soft: Ticket.RequesterGuid == bot when ticket present.
            if (!parsed.Ticket.RequesterGuid.IsEmpty() && parsed.Ticket.RequesterGuid != botGuid &&
                Playerbots::GetLogLevel() >= 1)
            {
                TC_LOG_DEBUG("playerbots.packet",
                    "BotPacketParse SMSG_LFG_PROPOSAL_UPDATE soft Ticket.RequesterGuid mismatch bot={} ticket={}",
                    bot->GetName(),
                    parsed.Ticket.RequesterGuid.ToString());
            }

            if (Playerbots::GetLogLevel() >= 1)
                TC_LOG_DEBUG("playerbots.packet",
                    "BotPacketParse SMSG_LFG_PROPOSAL_UPDATE ok bot={} proposalId={} slot={} players={} ticket={}",
                    bot->GetName(),
                    parsed.ProposalID,
                    parsed.Slot,
                    parsed.Players.size(),
                    parsed.Ticket.RequesterGuid.ToString());

            PendingLfgProposal pending;
            pending.Ticket = parsed.Ticket;
            pending.InstanceID = parsed.InstanceID;
            pending.ProposalID = parsed.ProposalID;
            SetPendingLfgProposal(std::move(pending));
            signal.Name = "lfg proposal";
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
