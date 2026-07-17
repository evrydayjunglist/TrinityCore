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

#include "NewRpgStrategy.h"

// Relevance layout mirrors AC's NewRpgStrategy: the status-update default runs first each tick
// (returns false when no transition happened, letting the engine fall through), status-gated
// actions sit at AC's 3.0. "quest giver" (opportunistic pickup/turn-in — AC calls
// SearchQuestGiverAndAcceptOrReward inside every RPG action) and "attack anything" (the kill
// role AC's always-on grind strategy provides) outrank the status machine so they preempt
// travel legs whenever they actually have something to do.
//
// "use quest object", "talk to quest npc" and "loot" join the same always-on interact band (AC runs
// its object-use, talk-to and loot behaviours alongside the RPG loop the same way). All bail out of
// combat via IsUseful, so their ordering vs "attack anything" only matters out of combat — there we
// want the bot to use a nearby quest object, talk to a nearby quest NPC and pick up quest drops
// before it wanders off, so they outrank the kill role. TALKTO is the dominant objective type in
// low-level chains (the whole orc/troll Valley of Trials intro), so this is the unlock that lets a
// freshly-questing bot actually progress instead of standing at the POI making no progress.
//
// The "release spirit" / "run to corpse" / "reclaim corpse" death band sits ABOVE the interact band
// (relevance 55/54/53 > "quest giver" 30) — bot death handling V1, the corpse run. Each is
// IsUseful() only in its matching death phase (dead-not-ghost / ghost-far-from-corpse /
// ghost-on-corpse-past-reclaim-delay), so exactly one is live at a time and only while the bot is
// dead; a live bot is completely unaffected (interact-band actions already bail when the bot is
// dead, and every death action bails when it's alive). Mirrors AC's DeadStrategy shape,
// TC-native/packetless — see playerbots-bot-death-corpse-run-handoff.md.
std::vector<NextAction> NewRpgStrategy::GetDefaultActions()
{
    return {
        NextAction("release spirit", 55.0f),
        NextAction("run to corpse", 54.0f),
        NextAction("reclaim corpse", 53.0f),
        NextAction("quest giver", 30.0f),
        NextAction("use quest object", 25.0f),
        NextAction("talk to quest npc", 24.0f),
        NextAction("loot", 22.0f),
        NextAction("attack anything", 20.0f),
        NextAction("new rpg status update", 11.0f)
    };
}

void NewRpgStrategy::InitTriggers(std::vector<TriggerNode*>& triggers)
{
    // Accept a pending res before the corpse-run death band (relevance 100 > release spirit 55).
    triggers.push_back(new TriggerNode("resurrect request signal", { NextAction("accept resurrect", 100.0f) }));
    triggers.push_back(new TriggerNode("resurrect request", { NextAction("accept resurrect", 100.0f) }));

    // Guild charter offer (AC "petition offer" → "petition sign") for random/newrpg bots.
    triggers.push_back(new TriggerNode("petition offer signal", { NextAction("petition sign", 100.0f) }));
    triggers.push_back(new TriggerNode("petition offer", { NextAction("petition sign", 100.0f) }));

    // AC trigger-name vocabulary; handlers point at this fork's registered action names.
    triggers.push_back(new TriggerNode("go grind status", { NextAction("new rpg go grind", 3.0f) }));
    triggers.push_back(new TriggerNode("go camp status", { NextAction("new rpg go camp", 3.0f) }));
    triggers.push_back(new TriggerNode("wander random status", { NextAction("wander", 3.0f) }));
    triggers.push_back(new TriggerNode("do quest status", { NextAction("new rpg do quest", 3.0f) }));
    triggers.push_back(new TriggerNode("wander npc status", { NextAction("new rpg wander npc", 3.0f) }));
    // RPG_REST has no trigger/action (AC parity) — it's a pure timed sit; NewRpgStatusUpdateAction
    // returns the bot to IDLE after the rest duration and the next status' movement stands it.
}
