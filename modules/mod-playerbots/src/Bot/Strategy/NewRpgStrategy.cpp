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
std::vector<NextAction> NewRpgStrategy::GetDefaultActions()
{
    return {
        NextAction("quest giver", 30.0f),
        NextAction("attack anything", 20.0f),
        NextAction("new rpg status update", 11.0f)
    };
}

void NewRpgStrategy::InitTriggers(std::vector<TriggerNode*>& triggers)
{
    // AC trigger-name vocabulary; handlers point at this fork's registered action names.
    triggers.push_back(new TriggerNode("go grind status", { NextAction("new rpg go grind", 3.0f) }));
    triggers.push_back(new TriggerNode("wander random status", { NextAction("wander", 3.0f) }));
    triggers.push_back(new TriggerNode("do quest status", { NextAction("new rpg do quest", 3.0f) }));
}
