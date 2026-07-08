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
#include "CellImpl.h"
#include "Creature.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "QuestDef.h"
#include "Unit.h"

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
            if (objective.Type != QUEST_OBJECTIVE_MONSTER)
                continue;

            // For a MONSTER objective ObjectID is the creature entry (unlike an ITEM objective,
            // whose ObjectID is an item id — kill-and-loot mobs are out of scope for V1, see the
            // handoff § 2).
            if (objective.ObjectID <= 0)
                continue;

            if (!bot->IsQuestObjectiveComplete(slot, quest, objective))
                entries.insert(uint32(objective.ObjectID));
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

    Creature* result = nullptr;
    float bestDistSq = (radius * radius) + 1.0f;
    for (Creature* creature : creatures)
    {
        float const distSq = bot->GetExactDistSq(creature);
        if (distSq < bestDistSq)
        {
            bestDistSq = distSq;
            result = creature;
        }
    }

    return result;
}
