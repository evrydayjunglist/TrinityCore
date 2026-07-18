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

#include "FollowMasterStrategy.h"

std::vector<NextAction> FollowMasterStrategy::GetDefaultActions()
{
    // "loot" find/open (SendLoot) outranks follow when a nearby corpse is worth taking;
    // store/money/release is signal-driven ("loot response" → "store loot").
    return {
        NextAction("loot", 22.0f),
        NextAction("follow", 1.0f)
    };
}

void FollowMasterStrategy::InitTriggers(std::vector<TriggerNode*>& triggers)
{
    // Auto-accept a pending party invite from the master. Two triggers fire the SAME accept action:
    //   1. "group invite signal" — SMSG_PARTY_INVITE observed via the packet-observation layer,
    //      reacted to on the very next tick (playerbots-bot-packet-observation-handoff.md § 5c).
    //   2. "group invite" — the original per-tick poll of Player::GetGroupInvite(), kept as a
    //      fallback so the invite is still accepted if the signal is ever lost (bounded queue) or
    //      packet observation is disabled. Accept is idempotent (the second attempt finds no
    //      pending invite), so double-firing is benign — "we don't break things".
    // Both at high relevance so they preempt the default "follow" the tick the invite lands.
    triggers.push_back(new TriggerNode("group invite signal", { NextAction("accept invitation", 100.0f) }));
    triggers.push_back(new TriggerNode("group invite", { NextAction("accept invitation", 100.0f) }));

    // Guild invite into the master's guild (AC WorldPacketHandlerStrategy "guild invite" →
    // "guild accept"). Signal + GetGuildIdInvited() poll; accept is idempotent.
    triggers.push_back(new TriggerNode("guild invite signal", { NextAction("guild accept", 100.0f) }));
    triggers.push_back(new TriggerNode("guild invite", { NextAction("guild accept", 100.0f) }));

    // Resurrect request while dead (AC DeadStrategy "accept resurrect"). Master-alt never runs
    // NewRpg's death band (HasMaster short-circuit), so follow must own this path.
    triggers.push_back(new TriggerNode("resurrect request signal", { NextAction("accept resurrect", 100.0f) }));
    triggers.push_back(new TriggerNode("resurrect request", { NextAction("accept resurrect", 100.0f) }));

    // Master-offered guild charter (AC WorldPacketHandlerStrategy "petition offer" →
    // "petition sign"). Signal + weak stash poll; Choice = 0 via HandleSignPetition.
    triggers.push_back(new TriggerNode("petition offer signal", { NextAction("petition sign", 100.0f) }));
    triggers.push_back(new TriggerNode("petition offer", { NextAction("petition sign", 100.0f) }));

    // Vendor buy failed (AC WorldPacketHandlerStrategy "not enough money" /
    // "not enough reputation" → tell). Signal-only after SMSG_BUY_FAILED Reason dispatch;
    // no Player pending-buy poll dual.
    triggers.push_back(new TriggerNode("not enough money", { NextAction("tell not enough money", 100.0f) }));
    triggers.push_back(new TriggerNode("not enough reputation", { NextAction("tell not enough reputation", 100.0f) }));

    // Party leader promoted (AC WorldPacketHandlerStrategy "group set leader" → "reset botAI").
    // Signal-only after SMSG_GROUP_NEW_LEADER Layer-2 OK; no poll dual for leader name alone.
    triggers.push_back(new TriggerNode("group set leader", { NextAction("reset botAI", 100.0f) }));

    // Master mount sync wake-up (AC WorldPacketHandlerStrategy "check mount state").
    // Signal-only after SMSG_MOVE_SET_RUN_SPEED Layer-2 OK; no NonCombat timer poll in V1.
    triggers.push_back(new TriggerNode("check mount state", { NextAction("check mount state", 100.0f) }));

    // Equip / inventory failure (AC WorldPacketHandlerStrategy "cannot equip" →
    // "tell cannot equip"). Signal-only after SMSG_INVENTORY_CHANGE_FAILURE Layer-2 OK
    // with a V1 message-map hit; AC duplicate name "inventory change failure" not wired.
    triggers.push_back(new TriggerNode("cannot equip", { NextAction("tell cannot equip", 100.0f) }));

    // Trade propose/accept (AC WorldPacketHandlerStrategy "trade status" → "accept trade").
    // Signal-only after SMSG_TRADE_STATUS Layer-2 OK for PROPOSED/ACCEPTED; no poll dual.
    triggers.push_back(new TriggerNode("trade status", { NextAction("accept trade", 100.0f) }));

    // Trade item/gold list update (AC WorldPacketHandlerStrategy "trade status extended").
    // Signal-only after SMSG_TRADE_UPDATED Layer-2 OK; V1 TellMaster on locked NONTRADED only.
    triggers.push_back(new TriggerNode("trade status extended",
        { NextAction("trade status extended", 100.0f) }));

    // Acquired loot window (AC WorldPacketHandlerStrategy "loot response" → "store loot").
    // Signal-only after SMSG_LOOT_RESPONSE Layer-2 OK + Acquired; Handle* store, no poll dual.
    triggers.push_back(new TriggerNode("loot response", { NextAction("store loot", 100.0f) }));

    // Self item grant (AC WorldPacketHandlerStrategy "item push result" → quest tell;
    // unlock/open/equip chain out of scope). Signal-only after SMSG_ITEM_PUSH_RESULT Layer-2 OK.
    triggers.push_back(new TriggerNode("item push result",
        { NextAction("item push result", 100.0f) }));

    // Group Need/Greed result (AC WorldPacketHandlerStrategy "loot roll won" → equip upgrades;
    // equip out of scope). Signal-only after SMSG_LOOT_ROLL_WON Layer-2 OK; optional self-won tell.
    triggers.push_back(new TriggerNode("loot roll won",
        { NextAction("loot roll won", 100.0f) }));

    // Group Need/Greed roll start (AC WorldPacketHandlerStrategy "master loot roll").
    // Signal-only after SMSG_START_LOOT_ROLL Layer-2 OK; V1 Pass via HandleLootRoll.
    triggers.push_back(new TriggerNode("master loot roll",
        { NextAction("master loot roll", 100.0f) }));

    // Party op result (AC WorldPacketHandlerStrategy "party command").
    // Explicit TriggerNode (do not rely on AC supported-list-only quirk). Signal-only after
    // SMSG_PARTY_COMMAND_RESULT Layer-2 OK; optional leave via HandleLeaveGroupOpcode.
    triggers.push_back(new TriggerNode("party command",
        { NextAction("party command", 100.0f) }));

    // Level-up (AC WorldPacketHandlerStrategy "levelup" → auto maintenance; maintenance out of
    // scope). Signal-only after SMSG_LEVEL_UP_INFO Layer-2 OK; optional TellMaster.
    triggers.push_back(new TriggerNode("levelup",
        { NextAction("levelup", 100.0f) }));

    // XP grant log (AC WorldPacketHandlerStrategy "xpgain" → "xp gain"; GiveXP re-apply /
    // kill broadcast out of scope). Signal+action both "xpgain" after SMSG_LOG_XP_GAIN
    // Layer-2 OK; optional TellMaster.
    triggers.push_back(new TriggerNode("xpgain",
        { NextAction("xpgain", 100.0f) }));

    // Cast fail (AC creators "cast failed" → "tell cast failed"; AC strategy omits
    // TriggerNode — wire FollowMaster anyway). Signal after SMSG_CAST_FAILED Layer-2 OK;
    // TellMaster Reason subset only when CalcCastTime >= 2000.
    triggers.push_back(new TriggerNode("cast failed",
        { NextAction("tell cast failed", 100.0f) }));

    // Inbound text + oneshot emotes (AC EmoteStrategy both → "emote"; ReceiveEmote matrix
    // out of scope). Signal+action both names after Layer-2 OK; TellMaster when source == master.
    triggers.push_back(new TriggerNode("receive text emote",
        { NextAction("receive text emote", 100.0f) }));
    triggers.push_back(new TriggerNode("receive emote",
        { NextAction("receive emote", 100.0f) }));
}
