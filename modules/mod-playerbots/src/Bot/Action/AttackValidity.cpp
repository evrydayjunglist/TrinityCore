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

#include "AttackValidity.h"
#include "Bot/Rpg/QuestItemDropCache.h"
#include "CellImpl.h"
#include "Creature.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "QuestDef.h"
#include "SafeMovement.h"
#include "Unit.h"
#include <algorithm>
#include <utility>
#include <vector>

bool IsValidAttackTarget(Player* bot, Unit* target)
{
    if (!bot || !target || !target->IsInWorld())
        return false;

    if (target->isDead())
        return false;

    if (bot->IsFriendlyTo(target))
        return false;

    if (!bot->IsValidAttackTarget(target))
        return false;

    if (!bot->IsWithinLOSInMap(target))
        return false;

    return true;
}

Unit* FindNearbyAttackableUnit(Player* bot, float radius)
{
    if (!bot)
        return nullptr;

    Unit* victim = nullptr;
    Trinity::NearestAttackableUnitInObjectRangeCheck check(bot, bot, radius);
    Trinity::UnitLastSearcher<Trinity::NearestAttackableUnitInObjectRangeCheck> searcher(bot, victim, check);
    Cell::VisitAllObjects(bot, searcher, radius);

    if (victim && IsValidAttackTarget(bot, victim))
        return victim;

    return nullptr;
}

void CollectQuestKillEntries(Player* bot, std::unordered_set<uint32_t>& entries)
{
    if (!bot)
        return;

    for (uint16 slot = 0; slot < MAX_QUEST_LOG_SIZE; ++slot)
    {
        uint32 const questId = bot->GetQuestSlotQuestId(slot);
        if (!questId)
            continue;

        if (bot->GetQuestStatus(questId) != QUEST_STATUS_INCOMPLETE)
            continue;

        Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
        if (!quest)
            continue;

        for (QuestObjective const& objective : quest->Objectives)
        {
            if (objective.ObjectID <= 0)
                continue;

            if (bot->IsQuestObjectiveComplete(slot, quest, objective))
                continue;

            if (objective.Type == QUEST_OBJECTIVE_MONSTER)
            {
                // For a MONSTER objective ObjectID is the creature entry.
                entries.insert(uint32(objective.ObjectID));
            }
            else if (objective.Type == QUEST_OBJECTIVE_ITEM)
            {
                // Convergence F4 (playerbots-rpg-quest-convergence-fixes-handoff.md § 4-F4): for an
                // ITEM objective ObjectID is an item id — arm the kill search with the creatures
                // that drop it as quest loot (creature_questitem reverse index), so kill-and-loot
                // quests get their killing half (LootAction already handles the looting half).
                // Items gathered from gameobjects naturally map to no creatures and stay with the
                // UseQuestObjectAction gather path.
                if (std::unordered_set<uint32> const* sources = sQuestItemDropCache->GetDropSources(uint32(objective.ObjectID)))
                    entries.insert(sources->begin(), sources->end());
            }
        }
    }
}

namespace
{
class QuestKillTargetCheck
{
public:
    QuestKillTargetCheck(Player* bot, float range, std::unordered_set<uint32_t> const* entries)
        : _bot(bot), _range(range), _entries(entries) { }

    bool operator()(Creature* creature) const
    {
        if (!_entries->contains(creature->GetEntry()))
            return false;

        if (!creature->IsInWorld() || !creature->IsAlive())
            return false;

        // Per-candidate accept test is the module's neutral-allowing validity check (not the
        // hostile-only core search) — that's the whole point of this searcher.
        if (!IsValidAttackTarget(_bot, creature))
            return false;

        return _bot->IsWithinDist(creature, _range);
    }

private:
    Player* _bot;
    float _range;
    std::unordered_set<uint32_t> const* _entries;
};
} // namespace

Unit* FindNearbyQuestKillTarget(Player* bot, float radius)
{
    if (!bot)
        return nullptr;

    std::unordered_set<uint32_t> entries;
    CollectQuestKillEntries(bot, entries);
    if (entries.empty())
        return nullptr;

    std::list<Creature*> creatures;
    QuestKillTargetCheck check(bot, radius, &entries);
    Trinity::CreatureListSearcher<QuestKillTargetCheck> searcher(bot, creatures, check);
    Cell::VisitAllObjects(bot, searcher, radius);

    if (creatures.empty())
        return nullptr;

    std::vector<std::pair<float, Creature*>> byDistance;
    byDistance.reserve(creatures.size());
    for (Creature* creature : creatures)
        byDistance.push_back({ bot->GetExactDistSq(creature), creature });
    std::sort(byDistance.begin(), byDistance.end(),
        [](auto const& a, auto const& b) { return a.first < b.first; });

    // Convergence F3 (playerbots-rpg-quest-convergence-fixes-handoff.md § 4-F3): prefer a candidate
    // the bot can actually walk to, so it doesn't latch onto a ridge-perched mob it can attack but
    // never reach (AttackAnythingAction's SafeMovement chase gate then refuses MoveChase and the bot
    // stands flailing out of range). Probe only the nearest few — mmap path queries are real work —
    // and if none pass, fall back to the plain nearest: never *fewer* targets than before this fix.
    constexpr size_t QUEST_KILL_REACHABILITY_PROBES = 5;
    size_t const probeCount = std::min(QUEST_KILL_REACHABILITY_PROBES, byDistance.size());
    for (size_t i = 0; i < probeCount; ++i)
    {
        Creature* candidate = byDistance[i].second;
        if (IsApproachPathWalkable(bot, candidate->GetPositionX(), candidate->GetPositionY(), candidate->GetPositionZ()))
            return candidate;
    }

    return byDistance.front().second;
}
