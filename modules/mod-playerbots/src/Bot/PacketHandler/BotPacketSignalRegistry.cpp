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
#include "Opcodes.h"
#include <unordered_map>

namespace Playerbots::PacketHandler
{
PacketSignalEntry const* LookupPacketSignal(uint32 opcode)
{
    // AC's botOutgoingPacketHandlers shape (mod-playerbots-master PlayerbotAI.cpp:178-223).
    // Signal name always; payload parse is opt-in per opcode behind the three-layer gate
    // (see playerbots-bot-packet-payload-parse-handoff.md).
    static std::unordered_map<uint32, PacketSignalEntry> const registry = {
        { SMSG_PARTY_INVITE, { "group invite signal", true, &HandlePartyInvite, false, nullptr } },
        { SMSG_GUILD_INVITE, { "guild invite signal", true, &HandleGuildInvite, false, nullptr } },
        { SMSG_RESURRECT_REQUEST, { "resurrect request signal", true, &HandleResurrectRequest, false, nullptr } },
        { SMSG_PETITION_SHOW_SIGNATURES, { "petition offer signal", true, &HandlePetitionShowSignatures, false, nullptr } },
        // Placeholder name rewritten in HandleBuyFailed to AC "not enough money" /
        // "not enough reputation" (or cleared for other BuyResult reasons).
        { SMSG_BUY_FAILED, { "buy failed", true, &HandleBuyFailed, false, nullptr } },
        // AC SMSG_GROUP_SET_LEADER → Midnight SMSG_GROUP_NEW_LEADER; Cleared unless Layer 2 OK
        // (and when PayloadParse is off — no poll dual for leader name alone).
        { SMSG_GROUP_NEW_LEADER, { "group set leader", true, &HandleGroupNewLeader, true, nullptr } },
        // AC SMSG_GROUP_DESTROYED → "group destroyed"; empty GroupDestroyed::Write(). Cleared
        // unless Layer 2 OK (GetGroup()==nullptr). Enable=0: no !GetGroup() poll (too noisy).
        { SMSG_GROUP_DESTROYED, { "group destroyed", true, &HandleGroupDestroyed, true, nullptr } },
        // AC SMSG_FORCE_RUN_SPEED_CHANGE → Midnight SMSG_MOVE_SET_RUN_SPEED; Cleared unless
        // Layer 2 OK (and when PayloadParse is off — no poll dual for "speed just changed").
        { SMSG_MOVE_SET_RUN_SPEED, { "check mount state", true, &HandleMoveSetRunSpeed, true, nullptr } },
        // AC registered the same opcode twice ("cannot equip" / "inventory change failure").
        // One TC SMSG; V1 fires the strategy-wired name only. Cleared unless Layer 2 OK.
        { SMSG_INVENTORY_CHANGE_FAILURE, { "cannot equip", true, &HandleInventoryChangeFailure, true, &BotPlayerbotAI::ClearPendingCannotEquipTell } },
        // Cleared unless Layer 2 OK and Status is PROPOSED/ACCEPTED (V1 begin/accept).
        // Enable=0 / other statuses: no poll dual for trade window.
        { SMSG_TRADE_STATUS, { "trade status", true, &HandleTradeStatus, true, &BotPlayerbotAI::ClearPendingTradeStatus } },
        // AC SMSG_TRADE_STATUS_EXTENDED → Midnight SMSG_TRADE_UPDATED; Cleared unless Layer 2 OK
        // (and when PayloadParse is off — no poll dual for trade item/gold list).
        { SMSG_TRADE_UPDATED, { "trade status extended", true, &HandleTradeUpdated, true, &BotPlayerbotAI::ClearPendingTradeUpdatedLockedTell } },
        // Cleared unless Layer 2 OK and Acquired (V1 store). Enable=0 / !Acquired: no poll dual.
        { SMSG_LOOT_RESPONSE, { "loot response", true, &HandleLootResponse, true, &BotPlayerbotAI::ClearPendingLootStore } },
        // Cleared unless Layer 2 OK on self inventory dual. Enable=0 / party broadcast: no poll dual.
        { SMSG_ITEM_PUSH_RESULT, { "item push result", true, &HandleItemPushResult, true, &BotPlayerbotAI::ClearPendingItemPush } },
        // Cleared unless Layer 2 OK (RollType/Winner + soft group/GetLootRoll). Enable=0: no poll dual.
        { SMSG_LOOT_ROLL_WON, { "loot roll won", true, &HandleLootRollWon, true, &BotPlayerbotAI::ClearPendingLootRollWon } },
        // Cleared unless Layer 2 OK (GROUP/NBG Method + live GetLootRoll). Enable=0: no poll dual.
        { SMSG_START_LOOT_ROLL, { "master loot roll", true, &HandleStartLootRoll, true, &BotPlayerbotAI::ClearPendingMasterLootRoll } },
        // Cleared unless Layer 2 OK (known Command/Result). Enable=0: no poll dual.
        { SMSG_PARTY_COMMAND_RESULT, { "party command", true, &HandlePartyCommandResult, true, &BotPlayerbotAI::ClearPendingPartyCommand } },
        // Cleared unless Layer 2 OK (Level bounds + GetLevel dual). Enable=0: no poll dual.
        { SMSG_LEVEL_UP_INFO, { "levelup", true, &HandleLevelUpInfo, true, &BotPlayerbotAI::ClearPendingLevelUp } },
        // AC SMSG_LOG_XPGAIN → Midnight SMSG_LOG_XP_GAIN; Cleared unless Layer 2 OK
        // (Reason + Amount/Original). Enable=0: no poll dual. Signal+action both "xpgain".
        { SMSG_LOG_XP_GAIN, { "xpgain", true, &HandleLogXpGain, true, &BotPlayerbotAI::ClearPendingXpGain } },
        // Cleared unless Layer 2 OK (Reason range + known SpellID). Enable=0: no poll dual.
        // Signal "cast failed" → action "tell cast failed" (AC names; AC strategy TriggerNode gap).
        { SMSG_CAST_FAILED, { "cast failed", true, &HandleCastFailed, true, &BotPlayerbotAI::ClearPendingCastFailed } },
        // Cleared unless Layer 2 OK (source sanity + soft player/NPC/self). Enable=0: no poll dual.
        // Signal+action both "receive text emote" / "receive emote" (no EmoteStrategy / ReceiveEmote matrix).
        { SMSG_TEXT_EMOTE, { "receive text emote", true, &HandleTextEmote, true, &BotPlayerbotAI::ClearPendingReceiveEmote } },
        { SMSG_EMOTE, { "receive emote", true, &HandleEmote, true, &BotPlayerbotAI::ClearPendingReceiveEmote } },
        // Cleared unless Layer 2 OK + master challenger (V1 accept). Enable=0: poll twin may still accept.
        // Soft-skip when bot is Initiator (self-observation). Signal "duel requested" → "accept duel".
        { SMSG_DUEL_REQUESTED, { "duel requested", true, &HandleDuelRequested, true, &BotPlayerbotAI::ClearPendingDuelArbiter } },
        // Cleared unless Layer 2 OK (sLFGMgr ROLECHECK / PROPOSAL). Enable=0: proposal poll twin may accept if stash present.
        { SMSG_LFG_ROLE_CHECK_UPDATE, { "lfg role check", true, &HandleLfgRoleCheckUpdate, true, nullptr } },
        { SMSG_LFG_PROPOSAL_UPDATE, { "lfg proposal", true, &HandleLfgProposalUpdate, true, &BotPlayerbotAI::ClearPendingLfgProposal } },
        // Cleared unless Layer 2 OK (active quest / COMPLETE). Enable=0: no poll dual.
        { SMSG_QUEST_UPDATE_COMPLETE, { "quest update complete", true, &HandleQuestUpdateComplete, true, &BotPlayerbotAI::ClearPendingQuestUpdateComplete } },
        // AC "quest update add kill" → Midnight SMSG_QUEST_UPDATE_ADD_CREDIT; keep AC signal name.
        // Cleared unless Layer 2 OK (IsActiveQuest). Enable=0: no poll dual.
        { SMSG_QUEST_UPDATE_ADD_CREDIT, { "quest update add kill", true, &HandleQuestUpdateAddCredit, true, &BotPlayerbotAI::ClearPendingQuestUpdateAddKill } },
        // Cleared unless Layer 2 OK (GetSharedQuestID + InitiatedBy). Enable=0: confirm poll twin may fire.
        { SMSG_QUEST_CONFIRM_ACCEPT, { "confirm quest", true, &HandleQuestConfirmAccept, true, &BotPlayerbotAI::ClearPendingQuestConfirm } },
    };

    auto itr = registry.find(opcode);
    return itr != registry.end() ? &itr->second : nullptr;
}
} // namespace Playerbots::PacketHandler
