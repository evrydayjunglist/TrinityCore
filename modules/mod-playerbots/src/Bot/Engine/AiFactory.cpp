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

#include "AiFactory.h"
#include "AiObjectContext.h"
#include "Bot/Action/AttackAction.h"
#include "Bot/Action/AttackAnythingAction.h"
#include "Bot/Action/DeathActions.h"
#include "Bot/Action/FollowAction.h"
#include "Bot/Action/GroupActions.h"
#include "Bot/Action/GuildActions.h"
#include "Bot/Action/LootAction.h"
#include "Bot/Action/MountActions.h"
#include "Bot/Action/PetitionActions.h"
#include "Bot/Action/ResurrectActions.h"
#include "Bot/Action/ItemPushResultAction.h"
#include "Bot/Action/LootRollWonAction.h"
#include "Bot/Action/MasterLootRollAction.h"
#include "Bot/Action/PartyCommandAction.h"
#include "Bot/Action/LevelUpAction.h"
#include "Bot/Action/XpGainAction.h"
#include "Bot/Action/TellCastFailedAction.h"
#include "Bot/Action/ReceiveEmoteAction.h"
#include "Bot/Action/TellMasterActions.h"
#include "Bot/Action/TradeActions.h"
#include "Bot/Action/NewRpgActions.h"
#include "Bot/Action/QuestGiverAction.h"
#include "Bot/Action/TalkToQuestNpcAction.h"
#include "Bot/Action/UseQuestObjectAction.h"
#include "Bot/Action/WanderAction.h"
#include "Bot/Strategy/CombatStrategy.h"
#include "Bot/Strategy/FollowMasterStrategy.h"
#include "Bot/Strategy/NewRpgStrategy.h"
#include "Bot/Strategy/PassiveStrategy.h"
#include "Bot/Trigger/GroupTriggers.h"
#include "Bot/Trigger/GuildTriggers.h"
#include "Bot/Trigger/NewRpgTriggers.h"
#include "Bot/Trigger/PetitionTriggers.h"
#include "Bot/Trigger/ResurrectTriggers.h"
#include "Bot/Trigger/SignalTrigger.h"
#include "BotPlayerbotAI.h"
#include "DB2Stores.h"
#include "Log.h"
#include "Player.h"
#include "PlayerbotsConfig.h"

std::unique_ptr<AiObjectContext> AiFactory::CreateContext(BotPlayerbotAI* botAI, Player* player)
{
    auto context = std::make_unique<AiObjectContext>(botAI);
    context->RegisterStrategy("passive", std::make_unique<PassiveStrategy>(botAI));
    context->RegisterStrategy("follow", std::make_unique<FollowMasterStrategy>(botAI));
    context->RegisterStrategy("attack", std::make_unique<CombatStrategy>(botAI));
    context->RegisterStrategy("newrpg", std::make_unique<NewRpgStrategy>(botAI));
    context->RegisterAction("follow", std::make_unique<FollowAction>(botAI));
    context->RegisterAction("attack my target", std::make_unique<AttackMyTargetAction>(botAI));
    context->RegisterAction("accept invitation", std::make_unique<AcceptInvitationAction>(botAI));
    context->RegisterAction("wander", std::make_unique<WanderAction>(botAI));
    context->RegisterAction("quest giver", std::make_unique<QuestGiverAction>(botAI));

    // Master-alt party join (bot auto-accepts its master's invite). Two paths both fire the
    // "accept invitation" action (see FollowMasterStrategy::InitTriggers):
    //   - "group invite signal": the packet-observation layer's consume-on-read SignalTrigger,
    //     fired when SMSG_PARTY_INVITE is observed for the bot (reacts on the very next tick). Its
    //     name matches the opcode registry entry in BotPlayerbotAI.cpp (LookupPacketSignal).
    //   - "group invite": the original per-tick poll of Player::GetGroupInvite(), kept as a
    //     fallback so an invite is still accepted if a signal is ever lost or observation is off.
    // See playerbots-bot-packet-observation-handoff.md § 5c.
    context->RegisterTrigger("group invite signal", std::make_unique<SignalTrigger>(botAI, "group invite signal"));
    context->RegisterTrigger("group invite", std::make_unique<GroupInviteTrigger>(botAI));

    // Master-alt guild join — same dual signal/poll shape as party invite (AC: "guild invite"
    // → "guild accept"). Layered payload parse for SMSG_GUILD_INVITE is in BotPlayerbotAI.
    context->RegisterAction("guild accept", std::make_unique<GuildAcceptAction>(botAI));
    context->RegisterTrigger("guild invite signal", std::make_unique<SignalTrigger>(botAI, "guild invite signal"));
    context->RegisterTrigger("guild invite", std::make_unique<GuildInviteTrigger>(botAI));

    // Accept pending resurrect while dead (AC: DeadStrategy "accept resurrect"). Signal +
    // IsResurrectRequested() poll; HandleResurrectResponse Response = 0 (Midnight, not AC uint8(1)).
    // Wired on follow / newrpg / passive — NewRpg death band skips HasMaster(), so master-alt
    // needs FollowMasterStrategy; solo random needs newrpg/passive.
    context->RegisterAction("accept resurrect", std::make_unique<AcceptResurrectAction>(botAI));
    context->RegisterTrigger("resurrect request signal",
        std::make_unique<SignalTrigger>(botAI, "resurrect request signal"));
    context->RegisterTrigger("resurrect request", std::make_unique<ResurrectRequestTrigger>(botAI));

    // Guild charter sign (AC: "petition offer" → "petition sign"). Layered parse stashes Item
    // GUID; HandleSignPetition Choice = 0. Wired follow + newrpg + passive: same-account
    // master-alts cannot use client Request Signature; GM `.playerbot login` uses +passive.
    context->RegisterAction("petition sign", std::make_unique<PetitionSignAction>(botAI));
    context->RegisterTrigger("petition offer signal",
        std::make_unique<SignalTrigger>(botAI, "petition offer signal"));
    context->RegisterTrigger("petition offer", std::make_unique<PetitionOfferTrigger>(botAI));

    // Vendor buy-failed tell-master (AC: "not enough money" / "not enough reputation" →
    // TellMasterAction fixed strings). One TC opcode SMSG_BUY_FAILED; Reason dispatch in
    // ProcessPayloadOnTick. FollowMaster V1 only (master-alt vendor playtest).
    context->RegisterAction("tell not enough money",
        std::make_unique<TellMasterAction>(botAI, "tell not enough money", "Not enough money"));
    context->RegisterAction("tell not enough reputation",
        std::make_unique<TellMasterAction>(botAI, "tell not enough reputation", "Not enough reputation"));
    context->RegisterTrigger("not enough money",
        std::make_unique<SignalTrigger>(botAI, "not enough money"));
    context->RegisterTrigger("not enough reputation",
        std::make_unique<SignalTrigger>(botAI, "not enough reputation"));

    // Party leader change (AC: "group set leader" → "reset botAI"). Midnight opcode is
    // SMSG_GROUP_NEW_LEADER; minimal ResetAiAction = ResetStrategies() only. FollowMaster V1.
    context->RegisterAction("reset botAI", std::make_unique<ResetAiAction>(botAI));
    context->RegisterTrigger("group set leader",
        std::make_unique<SignalTrigger>(botAI, "group set leader"));

    // Master-alt mount sync (AC: "check mount state"). Midnight wake-up is
    // SMSG_MOVE_SET_RUN_SPEED; minimal CheckMountStateAction. FollowMaster V1.
    context->RegisterAction("check mount state", std::make_unique<CheckMountStateAction>(botAI));
    context->RegisterTrigger("check mount state",
        std::make_unique<SignalTrigger>(botAI, "check mount state"));

    // Inventory / equip failure tell (AC: "cannot equip" → "tell cannot equip";
    // duplicate AC row "inventory change failure" = same reaction). One TC opcode
    // SMSG_INVENTORY_CHANGE_FAILURE; V1 message subset + pending tell. FollowMaster V1.
    context->RegisterAction("tell cannot equip", std::make_unique<TellCannotEquipAction>(botAI));
    context->RegisterTrigger("cannot equip",
        std::make_unique<SignalTrigger>(botAI, "cannot equip"));

    // Trade window (AC: "trade status" → "accept trade"). Midnight SMSG_TRADE_STATUS;
    // V1 master-alt begin (PROPOSED) + accept (ACCEPTED) only. FollowMaster V1.
    context->RegisterAction("accept trade", std::make_unique<AcceptTradeAction>(botAI));
    context->RegisterTrigger("trade status",
        std::make_unique<SignalTrigger>(botAI, "trade status"));

    // Trade item/gold update (AC: "trade status extended"). Midnight SMSG_TRADE_UPDATED;
    // V1 locked NONTRADED TellMaster only (no pick-lock). FollowMaster V1.
    context->RegisterAction("trade status extended",
        std::make_unique<TradeStatusExtendedAction>(botAI));
    context->RegisterTrigger("trade status extended",
        std::make_unique<SignalTrigger>(botAI, "trade status extended"));

    // Loot window store (AC: "loot response" → "store loot"). Midnight SMSG_LOOT_RESPONSE;
    // HandleLootMoney / HandleAutostoreLootItem / HandleLootRelease. Wired follow + newrpg.
    // "loot" is find/approach/SendLoot open only — no packetless StoreLootItem drain.
    context->RegisterAction("store loot", std::make_unique<StoreLootAction>(botAI));
    context->RegisterTrigger("loot response",
        std::make_unique<SignalTrigger>(botAI, "loot response"));

    // Inventory item-granted wake-up (AC: "item push result" → quest tell; unlock/open/equip
    // out of scope). Midnight SMSG_ITEM_PUSH_RESULT; FollowMaster V1 signal + optional quest tell.
    context->RegisterAction("item push result", std::make_unique<ItemPushResultAction>(botAI));
    context->RegisterTrigger("item push result",
        std::make_unique<SignalTrigger>(botAI, "item push result"));

    // Group Need/Greed roll result (AC: "loot roll won" → equip upgrades; equip out of scope).
    // Midnight SMSG_LOOT_ROLL_WON; FollowMaster V1 signal + optional self-winner TellMaster.
    context->RegisterAction("loot roll won", std::make_unique<LootRollWonAction>(botAI));
    context->RegisterTrigger("loot roll won",
        std::make_unique<SignalTrigger>(botAI, "loot roll won"));

    // Group Need/Greed roll start (AC: "master loot roll" → CountRollVote matrix).
    // Midnight SMSG_START_LOOT_ROLL; FollowMaster V1 Pass via HandleLootRoll (master-safe).
    context->RegisterAction("master loot roll", std::make_unique<MasterLootRollAction>(botAI));
    context->RegisterTrigger("master loot roll",
        std::make_unique<SignalTrigger>(botAI, "master loot roll"));

    // Party op result (AC: "party command" → leave-follow). Midnight SMSG_PARTY_COMMAND_RESULT;
    // FollowMaster V1 signal + optional HandleLeaveGroup when LEAVE OK names master.
    context->RegisterAction("party command", std::make_unique<PartyCommandAction>(botAI));
    context->RegisterTrigger("party command",
        std::make_unique<SignalTrigger>(botAI, "party command"));

    // Level-up wake-up (AC: "levelup" → auto maintenance). Midnight SMSG_LEVEL_UP_INFO;
    // FollowMaster V1 signal + optional TellMaster only (no AutoMaintenanceOnLevelup paste).
    context->RegisterAction("levelup", std::make_unique<LevelUpAction>(botAI));
    context->RegisterTrigger("levelup",
        std::make_unique<SignalTrigger>(botAI, "levelup"));

    // XP grant log (AC: "xpgain" → "xp gain"). Midnight SMSG_LOG_XP_GAIN; FollowMaster V1
    // signal+action both "xpgain" + optional TellMaster only (no GiveXP re-apply / kill broadcast).
    context->RegisterAction("xpgain", std::make_unique<XpGainAction>(botAI));
    context->RegisterTrigger("xpgain",
        std::make_unique<SignalTrigger>(botAI, "xpgain"));

    // Cast fail (AC: "cast failed" → "tell cast failed"). Midnight SMSG_CAST_FAILED;
    // FollowMaster V1 (AC WorldPacketHandlerStrategy omits TriggerNode — wire anyway).
    // TellMaster Reason subset + CalcCastTime >= 2000 only; no SpellInterrupted.
    context->RegisterAction("tell cast failed", std::make_unique<TellCastFailedAction>(botAI));
    context->RegisterTrigger("cast failed",
        std::make_unique<SignalTrigger>(botAI, "cast failed"));

    // Inbound emotes (AC: both → "emote" / EmoteAction ReceiveEmote matrix — out of scope).
    // Midnight SMSG_TEXT_EMOTE + SMSG_EMOTE; FollowMaster V1 signal+action same names +
    // optional TellMaster when source == master only.
    context->RegisterAction("receive text emote",
        std::make_unique<ReceiveEmoteAction>(botAI, "receive text emote"));
    context->RegisterTrigger("receive text emote",
        std::make_unique<SignalTrigger>(botAI, "receive text emote"));
    context->RegisterAction("receive emote",
        std::make_unique<ReceiveEmoteAction>(botAI, "receive emote"));
    context->RegisterTrigger("receive emote",
        std::make_unique<SignalTrigger>(botAI, "receive emote"));

    // Quest loot open + object interaction (AC: OpenLootAction, InteractWithGameObject).
    context->RegisterAction("loot", std::make_unique<LootAction>(botAI));
    context->RegisterAction("use quest object", std::make_unique<UseQuestObjectAction>(botAI));
    context->RegisterAction("talk to quest npc", std::make_unique<TalkToQuestNpcAction>(botAI));

    // Bot death handling V1 — the corpse run (AC: DeadStrategy / ReleaseSpiritAction /
    // ReviveFromCorpseAction). Death-state-gated actions in the always-on band above the interact
    // band; packetless replication of HandleRepopRequest / HandleReclaimCorpse. Solo random bots
    // only (master-alt death is NYI — see playerbots-bot-death-corpse-run-handoff.md §2).
    context->RegisterAction("release spirit", std::make_unique<ReleaseSpiritAction>(botAI));
    context->RegisterAction("run to corpse", std::make_unique<RunToCorpseAction>(botAI));
    context->RegisterAction("reclaim corpse", std::make_unique<ReclaimCorpseAction>(botAI));

    // Gate 10b — RPG state machine (AC: NewRpgActionContext / NewRpgTriggerContext)
    context->RegisterAction("attack anything", std::make_unique<AttackAnythingAction>(botAI));
    context->RegisterAction("new rpg status update", std::make_unique<NewRpgStatusUpdateAction>(botAI));
    context->RegisterAction("new rpg go grind", std::make_unique<NewRpgGoGrindAction>(botAI));
    context->RegisterAction("new rpg go camp", std::make_unique<NewRpgGoCampAction>(botAI));
    context->RegisterAction("new rpg do quest", std::make_unique<NewRpgDoQuestAction>(botAI));
    context->RegisterAction("new rpg wander npc", std::make_unique<NewRpgWanderNpcAction>(botAI));
    context->RegisterTrigger("go grind status", std::make_unique<NewRpgStatusTrigger>(botAI, RPG_GO_GRIND));
    context->RegisterTrigger("go camp status", std::make_unique<NewRpgStatusTrigger>(botAI, RPG_GO_CAMP));
    context->RegisterTrigger("wander random status", std::make_unique<NewRpgStatusTrigger>(botAI, RPG_WANDER_RANDOM));
    context->RegisterTrigger("do quest status", std::make_unique<NewRpgStatusTrigger>(botAI, RPG_DO_QUEST));
    context->RegisterTrigger("wander npc status", std::make_unique<NewRpgStatusTrigger>(botAI, RPG_WANDER_NPC));

    if (Playerbots::GetLogLevel() >= 1 && player)
    {
        uint8 const playerClass = player->GetClass();
        ChrSpecialization spec = player->GetPrimarySpecialization();
        if (ChrSpecializationEntry const* specEntry = player->GetPrimarySpecializationEntry())
        {
            TC_LOG_DEBUG("playerbots", "AiFactory context bot={} class={} spec={} ({})",
                player->GetName(), playerClass, uint32(spec), specEntry->Name[LOCALE_enUS]);
        }
        else if (ChrSpecializationEntry const* defaultSpec = sDB2Manager.GetDefaultChrSpecializationForClass(playerClass))
        {
            TC_LOG_DEBUG("playerbots", "AiFactory context bot={} class={} defaultSpec={} ({})",
                player->GetName(), playerClass, defaultSpec->ID, defaultSpec->Name[LOCALE_enUS]);
        }
        else
        {
            TC_LOG_DEBUG("playerbots", "AiFactory context bot={} class={} spec=unknown",
                player->GetName(), playerClass);
        }
    }

    return context;
}
