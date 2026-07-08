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

#include "TalkToQuestNpcAction.h"
#include "BotPlayerbotAI.h"
#include "CellImpl.h"
#include "Creature.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "Log.h"
#include "MotionMaster.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "PlayerbotsConfig.h"
#include "QuestDef.h"
#include "SafeMovement.h"
#include <unordered_set>

namespace
{
// Opportunistic-nearby reach for spotting the talk target — wide enough that a bot arriving at the
// objective's quest POI (DoQuest travels it there) sees the NPC, not so wide it detours to far-off
// spawns. Same "nearby, not routed" spirit as QuestGiverAction / UseQuestObjectAction.
constexpr float QUEST_TALKTO_SEARCH_RADIUS = 60.0f;

// Creature entries the bot still needs to *talk to* for an incomplete objective in its own quest log.
void CollectQuestTalkToEntries(Player* bot, std::unordered_set<uint32>& entries)
{
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
            if (objective.Type != QUEST_OBJECTIVE_TALKTO)
                continue;

            if (objective.ObjectID <= 0)
                continue;

            if (!bot->IsQuestObjectiveComplete(slot, quest, objective))
                entries.insert(uint32(objective.ObjectID));
        }
    }
}

class QuestTalkToCreatureCheck
{
public:
    QuestTalkToCreatureCheck(Player* bot, float range, std::unordered_set<uint32> const* entries)
        : _bot(bot), _range(range), _entries(entries) { }

    bool operator()(Creature* creature) const
    {
        if (!_entries->contains(creature->GetEntry()))
            return false;

        // Talk targets are living, present NPCs. A dead/despawning creature can't be talked to; the
        // core credit would still fire, but mirroring the client (you talk to a live NPC) avoids
        // crediting off a corpse that happens to share the entry.
        if (!creature->IsInWorld() || !creature->IsAlive())
            return false;

        return _bot->IsWithinDist(creature, _range);
    }

private:
    Player* _bot;
    float _range;
    std::unordered_set<uint32> const* _entries;
};
} // namespace

Creature* TalkToQuestNpcAction::FindQuestTalkToCreature(Player* bot, float range)
{
    std::unordered_set<uint32> entries;
    CollectQuestTalkToEntries(bot, entries);
    if (entries.empty())
        return nullptr;

    std::list<Creature*> creatures;
    QuestTalkToCreatureCheck check(bot, range, &entries);
    Trinity::CreatureListSearcher<QuestTalkToCreatureCheck> searcher(bot, creatures, check);
    Cell::VisitAllObjects(bot, searcher, range);

    Creature* result = nullptr;
    float bestDistSq = (range * range) + 1.0f;
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

bool TalkToQuestNpcAction::IsUseful()
{
    Player* bot = GetBot();
    if (!bot || !bot->IsInWorld() || !bot->IsAlive() || bot->IsInCombat())
        return false;

    return FindQuestTalkToCreature(bot, QUEST_TALKTO_SEARCH_RADIUS) != nullptr;
}

bool TalkToQuestNpcAction::Execute(Event /*event*/)
{
    Player* bot = GetBot();
    if (!bot)
        return false;

    Creature* npc = FindQuestTalkToCreature(bot, QUEST_TALKTO_SEARCH_RADIUS);
    if (!npc)
        return false;

    // Walk into talk range if needed (SafeMovement contract — validated navmesh route only, same as
    // Wander/Grind/QuestGiver/Loot/UseQuestObject).
    if (bot->GetDistance(npc) > Playerbots::GetQuestTalkToDistance())
    {
        if (bot->GetMotionMaster()->GetCurrentMovementGeneratorType() == POINT_MOTION_TYPE)
            return false; // already walking somewhere this tick

        return TryMoveToValidatedPoint(bot, npc->GetPositionX(), npc->GetPositionY(), npc->GetPositionZ());
    }

    // Drive the exact core credit path the client's talk interaction resolves to
    // (Player::TalkedToCreature -> UpdateQuestObjectiveProgress(QUEST_OBJECTIVE_TALKTO, ...)).
    bot->TalkedToCreature(npc->GetEntry(), npc->GetGUID());
    _botAI->GetRpgStatistics().questNpcsTalkedTo++;
    TC_LOG_DEBUG("playerbots", "[New RPG] {} talked to quest npc {} (entry {})",
        bot->GetName(), npc->GetName(), npc->GetEntry());

    return true;
}
