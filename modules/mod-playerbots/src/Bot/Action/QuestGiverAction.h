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

#ifndef TRINITY_PLAYERBOT_QUEST_GIVER_ACTION_H
#define TRINITY_PLAYERBOT_QUEST_GIVER_ACTION_H

#include "NewRpgBaseAction.h"

class BotPlayerbotAI;
class WorldObject;

// Gate 10 — NewRpgStrategy "quest-giver" sub-behavior (AC: SearchQuestGiverAndAcceptOrReward).
// Live grid search for any nearby creature/GO with a quest relation to this bot; generic —
// no hardcoded quest id, works for whatever the world DB actually offers near the bot.
// Gate 10b: composes NewRpgBaseAction so accepts are filtered through IsQuestWorthDoing/
// IsQuestCapableDoing and the quest log gets OrganizeQuestLog hygiene before new pickups,
// exactly like AC's SearchQuestGiverAndAcceptOrReward + InteractWithNpcOrGameObjectForQuest.
// Convergence F1: candidates pass the same strict HasQuestToAcceptOrReward gate AC applies —
// a giver whose menu only holds the bot's own INCOMPLETE turn-ins is NOT a target (that loose
// menu-non-empty gate was the giver-anchor ping-pong bug — see
// playerbots-rpg-quest-convergence-fixes-handoff.md § 3a).
class QuestGiverAction : public NewRpgBaseAction
{
public:
    QuestGiverAction(BotPlayerbotAI* botAI) : NewRpgBaseAction(botAI, "quest giver") { }

    bool Execute(Event event) override;
    bool IsUseful() override;

private:
    bool InteractWithQuestGiver(WorldObject* questGiver);
    WorldObject* FindNearbyQuestGiver();
};

#endif
