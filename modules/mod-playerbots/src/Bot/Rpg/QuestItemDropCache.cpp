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

#include "QuestItemDropCache.h"
#include "DBCEnums.h"
#include "Log.h"
#include "ObjectMgr.h"
#include <mutex>

QuestItemDropCache* QuestItemDropCache::instance()
{
    static QuestItemDropCache instance;
    return &instance;
}

std::unordered_set<uint32> const* QuestItemDropCache::GetDropSources(uint32 itemId)
{
    // See _mutex in the header: this singleton is shared across parallel map-update threads.
    std::lock_guard<std::mutex> guard(_mutex);

    if (!_built)
        Build();

    // Safe to hand out past the lock: the index is built once and never mutated after, and
    // std::unordered_map does not invalidate element pointers on rehash.
    auto itr = _sourcesByItem.find(itemId);
    return itr != _sourcesByItem.end() ? &itr->second : nullptr;
}

void QuestItemDropCache::Build()
{
    // The creature quest-item store has no whole-map accessor, so invert it by iterating the
    // creature templates and querying each entry — a one-shot pass at first use. DIFFICULTY_NONE
    // covers open-world spawns (GetCreatureQuestItemList falls back through the difficulty chain
    // itself); dungeon-difficulty-only quest items aren't indexed, which is fine — bots don't run
    // dungeon content, and a miss only means that objective stays with the pre-F4 behavior.
    uint32 pairs = 0;
    for (auto const& [entry, creatureTemplate] : sObjectMgr->GetCreatureTemplates())
    {
        std::vector<uint32> const* items = sObjectMgr->GetCreatureQuestItemList(entry, DIFFICULTY_NONE);
        if (!items)
            continue;

        for (uint32 const itemId : *items)
        {
            _sourcesByItem[itemId].insert(entry);
            ++pairs;
        }
    }

    TC_LOG_DEBUG("playerbots", "QuestItemDropCache: built {} quest items from {} item-creature pairs",
        uint32(_sourcesByItem.size()), pairs);

    // Mark built only after the index is fully populated (removes the build TOCTOU; caller holds _mutex).
    _built = true;
}
