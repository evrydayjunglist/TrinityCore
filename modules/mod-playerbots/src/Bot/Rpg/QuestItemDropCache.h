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

#ifndef TRINITY_PLAYERBOT_QUEST_ITEM_DROP_CACHE_H
#define TRINITY_PLAYERBOT_QUEST_ITEM_DROP_CACHE_H

#include "Define.h"
#include <unordered_map>
#include <unordered_set>

// Convergence F4 (playerbots-rpg-quest-convergence-fixes-handoff.md § 4-F4) — the reverse index
// that lets QUEST_OBJECTIVE_ITEM objectives arm the quest-kill search: item id → creature entries
// that drop it as quest loot. Built lazily once from the ObjectMgr-loaded creature quest-item
// store (creature_questitem — already in RAM and hotfix-aware, so no live creature_loot_template
// scan), same one-shot cache pattern as GrindLocationCache/HubLocationCache. Data-first: no
// hardcoded ids anywhere; items whose sources are gameobjects simply have no entry here and stay
// with the UseQuestObjectAction gather path.
class QuestItemDropCache
{
public:
    static QuestItemDropCache* instance();

    // Creature entries that drop this item as quest loot; nullptr when no creature does (e.g. the
    // item is gathered from a gameobject). Builds the whole index on first call (world-restart
    // lifetime after that — quest-item relations only change with world data).
    std::unordered_set<uint32> const* GetDropSources(uint32 itemId);

private:
    QuestItemDropCache() = default;

    void Build();

    bool _built = false;
    std::unordered_map<uint32, std::unordered_set<uint32>> _sourcesByItem;
};

#define sQuestItemDropCache QuestItemDropCache::instance()

#endif
