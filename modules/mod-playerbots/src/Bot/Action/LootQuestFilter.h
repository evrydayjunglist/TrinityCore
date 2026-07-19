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

#ifndef TRINITY_PLAYERBOT_LOOT_QUEST_FILTER_H
#define TRINITY_PLAYERBOT_LOOT_QUEST_FILTER_H

#include "Define.h"

class Player;

namespace Playerbots
{
// Shared quest-relevance filter used by packetless LootAction and Handle*-backed StoreLootAction.
// An item is worth storing when it starts a quest or fills an incomplete ITEM objective.
bool IsQuestRelevantLootItem(Player* bot, uint32 itemId);
}

#endif
