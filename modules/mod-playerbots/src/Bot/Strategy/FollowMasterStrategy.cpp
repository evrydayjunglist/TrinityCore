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
    return { NextAction("follow", 1.0f) };
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
}
