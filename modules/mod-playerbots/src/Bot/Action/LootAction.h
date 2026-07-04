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

#ifndef TRINITY_PLAYERBOT_LOOT_ACTION_H
#define TRINITY_PLAYERBOT_LOOT_ACTION_H

#include "Action.h"

class BotPlayerbotAI;

// Quest looting for RPG bots. AC reference: mod-playerbots-master's OpenLootAction/StoreLootAction
// (src/Ai/Base/Actions/LootAction.cpp) + LootObjectStack — same "find nearest lootable, open it,
// store what's allowed" shape, but TC-native and packetless: it calls Player::SendLoot /
// Player::StoreLootItem / WorldSession::DoLootRelease directly instead of queueing synthetic
// CMSG_LOOT / CMSG_AUTOSTORE_LOOT_ITEM / CMSG_LOOT_RELEASE (a socketless bot never processes an
// inbound packet queue). Ownership/anti-ninja is delegated entirely to the core — the finder gates
// on Player::isAllowedToLoot and each slot on LootItem::GetUiTypeForPlayer, so a bot only ever takes
// what a real looting player at that corpse could. V1 default is quest-relevant items only
// (Playerbots.LootQuestItemsOnly) so looting never drifts into Gate 18 gear/vendor economy.
//
// It also drains any loot window already open on the bot (Player::GetAELootView): that's how loot
// from a gameobject the UseQuestObjectAction opened via GameObject::Use (which itself calls
// Player::SendLoot for chests/gathering nodes) gets stored — no separate GO-loot path needed.
class LootAction : public Action
{
public:
    explicit LootAction(BotPlayerbotAI* botAI) : Action(botAI, "loot") { }

    bool Execute(Event event) override;
    bool IsUseful() override;
};

#endif
