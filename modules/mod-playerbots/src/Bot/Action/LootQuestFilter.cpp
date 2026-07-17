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

#include "LootQuestFilter.h"
#include "ItemTemplate.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "QuestDef.h"

namespace Playerbots
{
bool IsQuestRelevantLootItem(Player* bot, uint32 itemId)
{
    if (!bot || !itemId)
        return false;

    if (ItemTemplate const* proto = sObjectMgr->GetItemTemplate(itemId))
        if (proto->GetStartQuest() != 0)
            return true;

    for (uint16 slot = 0; slot < MAX_QUEST_LOG_SIZE; ++slot)
    {
        uint32 const questId = bot->GetQuestSlotQuestId(slot);
        if (!questId)
            continue;

        Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
        if (!quest)
            continue;

        for (QuestObjective const& objective : quest->Objectives)
        {
            if (objective.Type != QUEST_OBJECTIVE_ITEM)
                continue;

            if (uint32(objective.ObjectID) != itemId)
                continue;

            if (!bot->IsQuestObjectiveComplete(slot, quest, objective))
                return true;
        }
    }

    return false;
}
}
