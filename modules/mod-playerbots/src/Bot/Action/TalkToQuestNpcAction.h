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

#ifndef TRINITY_PLAYERBOT_TALK_TO_QUEST_NPC_ACTION_H
#define TRINITY_PLAYERBOT_TALK_TO_QUEST_NPC_ACTION_H

#include "Action.h"

class BotPlayerbotAI;
class Creature;
class Player;

// Completes a QUEST_OBJECTIVE_TALKTO objective in the bot's log (the "go report to / speak with NPC"
// step that dominates low-level quest chains — the whole orc/troll Valley of Trials intro is talk-to).
// Same shape as UseQuestObjectAction (find the objective's target nearby, walk into range via the
// SafeMovement contract, then drive the core credit path packetlessly), but for a creature instead of
// a gameobject.
//
// Credit goes straight through the core: Player::TalkedToCreature(entry, guid) —
// UpdateQuestObjectiveProgress(QUEST_OBJECTIVE_TALKTO, ...) — the exact call the client's talk
// interaction resolves to. No synthetic gossip packet. Only fires for creatures whose entry matches an
// *incomplete* TALKTO objective the bot actually holds, so it can't over-credit or spam.
//
// Opportunistic-nearby like the other interact-band actions: it does not route the bot to a distant
// talk target on its own — DoQuest travels the bot onto the objective's POI (where the target NPC is),
// and this action closes the last gap and talks. Active routing to talk targets is future work.
class TalkToQuestNpcAction : public Action
{
public:
    explicit TalkToQuestNpcAction(BotPlayerbotAI* botAI) : Action(botAI, "talk to quest npc") { }

    bool Execute(Event event) override;
    bool IsUseful() override;

private:
    Creature* FindQuestTalkToCreature(Player* bot, float range);
};

#endif
