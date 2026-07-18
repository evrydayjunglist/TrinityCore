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

#ifndef TRINITY_PLAYERBOT_QUEST_UPDATE_ACTIONS_H
#define TRINITY_PLAYERBOT_QUEST_UPDATE_ACTIONS_H

#include "Action.h"

class BotPlayerbotAI;

// Minimal AC "quest update complete": signal wake-up + optional TellMaster plain QuestID.
// No FormatQuest / BroadcastHelper / RPG counters.
class QuestUpdateCompleteAction : public Action
{
public:
    explicit QuestUpdateCompleteAction(BotPlayerbotAI* botAI)
        : Action(botAI, "quest update complete") { }

    bool Execute(Event event) override;
    bool IsUseful() override;
};

// Minimal AC "quest update add kill": signal wake-up + optional TellMaster
// "quest <QuestID> <Count>/<Required>" (+ ObjectID). No FormatQuest / creature name paste.
class QuestUpdateAddKillAction : public Action
{
public:
    explicit QuestUpdateAddKillAction(BotPlayerbotAI* botAI)
        : Action(botAI, "quest update add kill") { }

    bool Execute(Event event) override;
    bool IsUseful() override;
};

// AC "confirm quest" prefer QuestConfirmAcceptAction shape: construct QuestConfirmAccept →
// HandleQuestConfirmAccept. Master-only V1. No AddQuest / QueuePacket / FormatQuest.
class ConfirmQuestAction : public Action
{
public:
    explicit ConfirmQuestAction(BotPlayerbotAI* botAI)
        : Action(botAI, "confirm quest") { }

    bool Execute(Event event) override;
    bool IsUseful() override;
};

#endif
