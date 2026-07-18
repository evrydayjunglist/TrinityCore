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

#ifndef TRINITY_BOT_PLAYERBOT_AI_H
#define TRINITY_BOT_PLAYERBOT_AI_H

#include "AiObjectContext.h"
#include "Bot/Rpg/NewRpgInfo.h"
#include "Engine.h"
#include "ObjectGuid.h"
#include "PlayerbotAIBase.h"
#include "SharedDefines.h"
#include "WorldPacket.h"
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

class AiObjectContext;
class Engine;

// Gate 6: bot AI with AC-shaped Engine + passive strategy.
class BotPlayerbotAI : public PlayerbotAIBase
{
public:
    explicit BotPlayerbotAI(Player* bot);
    ~BotPlayerbotAI() = default;

    void ResetStrategies();
    Engine* GetEngine() const { return _engine.get(); }
    AiObjectContext* GetAiObjectContext() const { return _context.get(); }

    Player* GetMaster() const { return _master; }
    void SetMaster(Player* master) { _master = master; }
    bool HasMaster() const { return _master != nullptr; }

    // Gate 10b — per-bot RPG state machine + lifecycle counters + abandon set (AC keeps the
    // same trio as public members rpgInfo/rpgStatistic/lowPriorityQuest on PlayerbotAI).
    NewRpgInfo& GetRpgInfo() { return _rpgInfo; }
    NewRpgStatistic& GetRpgStatistics() { return _rpgStatistic; }
    std::unordered_set<uint32>& GetLowPriorityQuests() { return _lowPriorityQuest; }

    // Quests proven impossible to complete for ANY player (a COMPLETE quest with no ender
    // anywhere in world data, e.g. the auto-granted junk 55660 "Time Trials"). Distinct from
    // the low-priority set (that = "tried a POI, stalled"; this = "provably unactionable").
    // In-memory only (owner directive: never DropQuest/AbandonQuest/DB-touch); per-session, so
    // it starts empty each login and a later data fix or new objective handler re-includes the
    // quest automatically. See playerbots-rpg-active-questgiver-seeking-handoff.md §4.
    std::unordered_set<uint32>& GetUnactionableQuests() { return _unactionableQuest; }

    // Packet-observation signal layer (playerbots-bot-packet-observation-handoff.md § 5;
    // payload gate: playerbots-bot-packet-payload-parse-handoff.md). Called from the module's
    // ServerScript observer on ARBITRARY map/World sender threads — so it does nothing but a
    // static opcode→signal registry test and, on a hit, a cheap bounded enqueue under
    // _signalMutex (optional WorldPacket copy when PayloadParse is enabled). Typed parse and
    // Layer-2 cross-checks run only on the bot's own tick.
    void HandleBotOutgoingPacket(WorldPacket const& packet);

    // Consume-on-read: tick-thread only (SignalTrigger::IsActive during Engine::DoNextAction). The
    // first trigger to test its signal name this tick claims it; returns false once consumed.
    bool ConsumeSignal(std::string const& name);

    // Minimal AC TellMaster shape: whisper to master. No-master → DEBUG only (no say spam).
    // Tick-thread only.
    bool TellMaster(std::string_view text);

    // Last Layer-2-OK SMSG_PETITION_SHOW_SIGNATURES item GUID (master-offered charter). Cleared
    // after PetitionSignAction runs. Tick-thread only.
    ObjectGuid GetPendingPetitionOffer() const { return _pendingPetitionOffer; }
    void SetPendingPetitionOffer(ObjectGuid guid) { _pendingPetitionOffer = guid; }
    void ClearPendingPetitionOffer() { _pendingPetitionOffer = ObjectGuid::Empty; }

    // Last V1 tell text from SMSG_INVENTORY_CHANGE_FAILURE Layer-2 OK. Cleared after
    // TellCannotEquipAction runs (or on Layer fail / Enable=0). Tick-thread only.
    std::string const& GetPendingCannotEquipTell() const { return _pendingCannotEquipTell; }
    void SetPendingCannotEquipTell(std::string text) { _pendingCannotEquipTell = std::move(text); }
    void ClearPendingCannotEquipTell() { _pendingCannotEquipTell.clear(); }

    // Last Layer-2-OK SMSG_TRADE_STATUS for V1 accept-trade (PROPOSED / ACCEPTED). Cleared
    // after AcceptTradeAction runs (or on Layer fail / Enable=0). Tick-thread only.
    std::optional<::TradeStatus> GetPendingTradeStatus() const { return _pendingTradeStatus; }
    void SetPendingTradeStatus(::TradeStatus status) { _pendingTradeStatus = status; }
    void ClearPendingTradeStatus() { _pendingTradeStatus.reset(); }

    // Locked TRADE_SLOT_NONTRADED tell from SMSG_TRADE_UPDATED Layer-2 OK (master trader).
    // Cleared after TradeStatusExtendedAction or on Layer fail / Enable=0. Tick-thread only.
    bool GetPendingTradeUpdatedLockedTell() const { return _pendingTradeUpdatedLockedTell; }
    void SetPendingTradeUpdatedLockedTell(bool pending) { _pendingTradeUpdatedLockedTell = pending; }
    void ClearPendingTradeUpdatedLockedTell() { _pendingTradeUpdatedLockedTell = false; }

    // Acquired SMSG_LOOT_RESPONSE stash for V1 "store loot" (Handle* path). Cleared after
    // StoreLootAction or on Layer fail / Enable=0 / !Acquired. Tick-thread only.
    struct PendingLootStore
    {
        ObjectGuid Owner;
        ObjectGuid LootObj;
        uint32 Coins = 0;
        struct ItemSlot
        {
            uint8 LootListID = 0;
            uint32 ItemID = 0;
            uint8 UIType = 0;
        };
        std::vector<ItemSlot> Items;
    };
    std::optional<PendingLootStore> const& GetPendingLootStore() const { return _pendingLootStore; }
    void SetPendingLootStore(PendingLootStore store) { _pendingLootStore = std::move(store); }
    void ClearPendingLootStore() { _pendingLootStore.reset(); }

    // Self SMSG_ITEM_PUSH_RESULT stash for V1 "item push result" quest TellMaster.
    // Cleared after ItemPushResultAction or on Layer fail / Enable=0 / non-self. Tick-thread only.
    struct PendingItemPush
    {
        uint32 ItemID = 0;
        int32 ProxyItemID = 0;
        int32 Quantity = 0;
        int32 QuantityInInventory = 0;
    };
    std::optional<PendingItemPush> const& GetPendingItemPush() const { return _pendingItemPush; }
    void SetPendingItemPush(PendingItemPush push) { _pendingItemPush = std::move(push); }
    void ClearPendingItemPush() { _pendingItemPush.reset(); }

    // SMSG_LOOT_ROLL_WON stash for V1 "loot roll won" optional self-winner TellMaster.
    // Cleared after LootRollWonAction or on Layer fail / Enable=0. Tick-thread only.
    struct PendingLootRollWon
    {
        ObjectGuid Winner;
        uint32 ItemID = 0;
    };
    std::optional<PendingLootRollWon> const& GetPendingLootRollWon() const { return _pendingLootRollWon; }
    void SetPendingLootRollWon(PendingLootRollWon pending) { _pendingLootRollWon = std::move(pending); }
    void ClearPendingLootRollWon() { _pendingLootRollWon.reset(); }

    // SMSG_START_LOOT_ROLL stash for V1 "master loot roll" Pass via HandleLootRoll.
    // Cleared after MasterLootRollAction or on Layer fail / Enable=0. Tick-thread only.
    struct PendingMasterLootRoll
    {
        ObjectGuid LootObj;
        uint8 LootListID = 0;
        uint32 ItemID = 0;
    };
    std::optional<PendingMasterLootRoll> const& GetPendingMasterLootRoll() const { return _pendingMasterLootRoll; }
    void SetPendingMasterLootRoll(PendingMasterLootRoll pending) { _pendingMasterLootRoll = std::move(pending); }
    void ClearPendingMasterLootRoll() { _pendingMasterLootRoll.reset(); }

    // SMSG_PARTY_COMMAND_RESULT stash for V1 "party command" optional leave-follow.
    // Cleared after PartyCommandAction or on Layer fail / Enable=0. Tick-thread only.
    struct PendingPartyCommand
    {
        uint8 Command = 0;
        uint8 Result = 0;
        std::string Name;
    };
    std::optional<PendingPartyCommand> const& GetPendingPartyCommand() const { return _pendingPartyCommand; }
    void SetPendingPartyCommand(PendingPartyCommand pending) { _pendingPartyCommand = std::move(pending); }
    void ClearPendingPartyCommand() { _pendingPartyCommand.reset(); }

    // SMSG_LEVEL_UP_INFO stash for V1 "levelup" optional TellMaster.
    // Cleared after LevelUpAction or on Layer fail / Enable=0. Tick-thread only.
    struct PendingLevelUp
    {
        int32 Level = 0;
    };
    std::optional<PendingLevelUp> const& GetPendingLevelUp() const { return _pendingLevelUp; }
    void SetPendingLevelUp(PendingLevelUp pending) { _pendingLevelUp = std::move(pending); }
    void ClearPendingLevelUp() { _pendingLevelUp.reset(); }

    // SMSG_LOG_XP_GAIN stash for V1 "xpgain" optional TellMaster.
    // Cleared after XpGainAction or on Layer fail / Enable=0. Tick-thread only.
    struct PendingXpGain
    {
        int32 Amount = 0;
    };
    std::optional<PendingXpGain> const& GetPendingXpGain() const { return _pendingXpGain; }
    void SetPendingXpGain(PendingXpGain pending) { _pendingXpGain = std::move(pending); }
    void ClearPendingXpGain() { _pendingXpGain.reset(); }

    // SMSG_CAST_FAILED stash for V1 "tell cast failed" optional TellMaster (≥2s gate).
    // Cleared after TellCastFailedAction or on Layer fail / Enable=0. Tick-thread only.
    struct PendingCastFailed
    {
        int32 SpellID = 0;
        int32 Reason = 0;
    };
    std::optional<PendingCastFailed> const& GetPendingCastFailed() const { return _pendingCastFailed; }
    void SetPendingCastFailed(PendingCastFailed pending) { _pendingCastFailed = std::move(pending); }
    void ClearPendingCastFailed() { _pendingCastFailed.reset(); }

    // SMSG_TEXT_EMOTE / SMSG_EMOTE stash for V1 "receive text emote" / "receive emote"
    // optional TellMaster when source == master. Cleared after ReceiveEmoteAction or on
    // Layer fail / Enable=0 / soft NPC-self skip. Tick-thread only.
    struct PendingReceiveEmote
    {
        ObjectGuid SourceGUID;
        uint32 EmoteID = 0;
        bool IsTextEmote = false;
    };
    std::optional<PendingReceiveEmote> const& GetPendingReceiveEmote() const { return _pendingReceiveEmote; }
    void SetPendingReceiveEmote(PendingReceiveEmote pending) { _pendingReceiveEmote = std::move(pending); }
    void ClearPendingReceiveEmote() { _pendingReceiveEmote.reset(); }

    // Last Layer-2-OK SMSG_DUEL_REQUESTED ArbiterGUID for V1 "accept duel". Cleared after
    // AcceptDuelAction or on Layer fail / Enable=0 / soft initiator skip. Tick-thread only.
    ObjectGuid GetPendingDuelArbiter() const { return _pendingDuelArbiter; }
    void SetPendingDuelArbiter(ObjectGuid guid) { _pendingDuelArbiter = guid; }
    void ClearPendingDuelArbiter() { _pendingDuelArbiter = ObjectGuid::Empty; }

protected:
    void UpdateAIInternal(uint32 diff) override;

private:
    std::unique_ptr<AiObjectContext> _context;
    std::unique_ptr<Engine> _engine;
    Player* _master = nullptr;
    NewRpgInfo _rpgInfo;
    NewRpgStatistic _rpgStatistic;
    std::unordered_set<uint32> _lowPriorityQuest;
    std::unordered_set<uint32> _unactionableQuest;

    struct QueuedSignal
    {
        std::string Name;
        std::optional<WorldPacket> Packet; // present when payload parse is enabled for this opcode
    };

    struct PendingSignal
    {
        std::string Name;
        uint8 TicksRemaining = 0;
    };

    void ProcessPayloadOnTick(QueuedSignal& signal);

    // Cross-thread signal handoff. _signalQueue is written by HandleBotOutgoingPacket on sender
    // threads and drained (swapped out) at the top of each tick under _signalMutex. _pendingSignals
    // is only ever touched on the bot's own tick thread (drain + consume + expire), so it needs no
    // lock of its own. TTL burns only on ticks where Engine::ProcessTriggers ran (not while GCD
    // blocks DoNextAction), so a follow/GCD window cannot expire signal-only reactions.
    std::mutex _signalMutex;
    std::vector<QueuedSignal> _signalQueue;
    std::vector<PendingSignal> _pendingSignals;
    ObjectGuid _pendingPetitionOffer;
    std::string _pendingCannotEquipTell;
    std::optional<::TradeStatus> _pendingTradeStatus;
    bool _pendingTradeUpdatedLockedTell = false;
    std::optional<PendingLootStore> _pendingLootStore;
    std::optional<PendingItemPush> _pendingItemPush;
    std::optional<PendingLootRollWon> _pendingLootRollWon;
    std::optional<PendingMasterLootRoll> _pendingMasterLootRoll;
    std::optional<PendingPartyCommand> _pendingPartyCommand;
    std::optional<PendingLevelUp> _pendingLevelUp;
    std::optional<PendingXpGain> _pendingXpGain;
    std::optional<PendingCastFailed> _pendingCastFailed;
    std::optional<PendingReceiveEmote> _pendingReceiveEmote;
    ObjectGuid _pendingDuelArbiter;
};

#endif
