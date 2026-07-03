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

#ifndef TRINITY_PLAYERBOT_NEW_RPG_ACTIONS_H
#define TRINITY_PLAYERBOT_NEW_RPG_ACTIONS_H

#include "NewRpgBaseAction.h"

class BotPlayerbotAI;

// Gate 10b — RPG state-machine actions. AC reference: mod-playerbots-master/src/Ai/World/Rpg/
// Action/NewRpgAction.h — same action names and transition rules, subset of AC's statuses.

// Runs every tick at the strategy's top default relevance; drives status transitions
// (Idle → weighted random pick, GoGrind arrival → WanderRandom, duration expiries → Idle).
// Returns false when no transition happened so lower-relevance status actions run this tick.
class NewRpgStatusUpdateAction : public NewRpgBaseAction
{
public:
    explicit NewRpgStatusUpdateAction(BotPlayerbotAI* botAI) : NewRpgBaseAction(botAI, "new rpg status update") { }

    bool Execute(Event event) override;
};

// RPG_GO_GRIND travel leg: MoveFarTo the chosen grind spot, small nudge on pathing failure so
// the next tick retries from a different position. Killing is the always-on "attack anything"
// action's job (AC delegates it to the grind strategy the same way).
class NewRpgGoGrindAction : public NewRpgBaseAction
{
public:
    explicit NewRpgGoGrindAction(BotPlayerbotAI* botAI) : NewRpgBaseAction(botAI, "new rpg go grind") { }

    bool Execute(Event event) override;
    bool IsUseful() override;
};

// RPG_DO_QUEST: travel to the pursued objective's POI (typed QuestObjective keys), hold with
// small wanders while kills/collections progress, resolve the next objective or the turn-in
// blob as state advances, and mark the quest low-priority (abandon set) after a full stay
// window with zero progress.
class NewRpgDoQuestAction : public NewRpgBaseAction
{
public:
    explicit NewRpgDoQuestAction(BotPlayerbotAI* botAI) : NewRpgBaseAction(botAI, "new rpg do quest") { }

    bool Execute(Event event) override;
    bool IsUseful() override;

private:
    bool DoIncompleteQuest(NewRpgInfo::DoQuest& data);
    bool DoCompletedQuest(NewRpgInfo::DoQuest& data);
};

#endif
