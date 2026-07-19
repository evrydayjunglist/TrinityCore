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

#include "ItemPushResultAction.h"
#include "BotPlayerbotAI.h"
#include "Log.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "PlayerbotsConfig.h"
#include "QuestDef.h"
#include <algorithm>
#include <sstream>

bool ItemPushResultAction::IsUseful()
{
    return _botAI && _botAI->GetBot() && _botAI->GetPendingItemPush().has_value();
}

bool ItemPushResultAction::Execute(Event /*event*/)
{
    if (!_botAI)
        return false;

    std::optional<BotPlayerbotAI::PendingItemPush> const pending = _botAI->GetPendingItemPush();
    _botAI->ClearPendingItemPush();
    if (!pending)
        return false;

    Player* bot = _botAI->GetBot();
    if (!bot)
        return false;

    // Signal wake-up always succeeds; TellMaster only when master present + objective advances.
    if (!_botAI->HasMaster())
        return true;

    auto matchesObjective = [&](uint32 itemId) -> bool
    {
        return itemId &&
            (itemId == pending->ItemID ||
                (pending->ProxyItemID != 0 && itemId == uint32(pending->ProxyItemID)));
    };

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

            if (!matchesObjective(uint32(objective.ObjectID)))
                continue;

            // AC: previousCount = itemCount - count; tell when previous was below required.
            int32 const previousCount = pending->QuantityInInventory - pending->Quantity;
            if (previousCount >= objective.Amount)
                continue;

            int32 const required = objective.Amount;
            int32 const available = std::min(pending->QuantityInInventory, required);
            uint32 const tellItemId = pending->ItemID ? pending->ItemID : uint32(pending->ProxyItemID);

            std::ostringstream out;
            out << tellItemId << ' ' << available << '/' << required << " quest " << questId;
            bool const told = _botAI->TellMaster(out.str());
            if (Playerbots::GetLogLevel() >= 1)
            {
                Player* master = _botAI->GetMaster();
                TC_LOG_DEBUG("playerbots", "ItemPushResultAction bot={} master={} text='{}' ok={}",
                    bot->GetName(),
                    master ? master->GetName() : "none",
                    out.str(),
                    told ? "yes" : "no");
            }

            // One tell per push (first matching incomplete objective).
            return true;
        }
    }

    return true;
}
