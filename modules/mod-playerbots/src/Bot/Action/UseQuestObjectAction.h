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

#ifndef TRINITY_PLAYERBOT_USE_QUEST_OBJECT_ACTION_H
#define TRINITY_PLAYERBOT_USE_QUEST_OBJECT_ACTION_H

#include "Action.h"

class BotPlayerbotAI;
class GameObject;
class Player;

// Uses a gameobject that satisfies a QUEST_OBJECTIVE_GAMEOBJECT objective in the bot's log (the
// "click the lever / open the cache" step of quest doing). AC reference: mod-playerbots-master's
// InteractWithGameObject / the GO half of OpenLootAction — same "find the objective GO, get in
// range, respect state + anti-ninja flags, then interact" shape, TC-native and packetless.
//
// Interaction goes straight through the core: it calls Player::GetGameObjectIfCanInteractWith
// (the exact gate WorldSession::HandleGameObjectUseOpcode uses) and then GameObject::Use(bot) — no
// synthetic CMSG_GAMEOBJ_USE. GameObject::Use fires the real quest credit, and for chest/gathering
// objectives it also opens the loot window via Player::SendLoot, which the LootAction then drains
// next tick — so no separate GO-loot code path is needed here.
class UseQuestObjectAction : public Action
{
public:
    explicit UseQuestObjectAction(BotPlayerbotAI* botAI) : Action(botAI, "use quest object") { }

    bool Execute(Event event) override;
    bool IsUseful() override;

private:
    GameObject* FindQuestObjectGameObject(Player* bot, float range);
};

#endif
