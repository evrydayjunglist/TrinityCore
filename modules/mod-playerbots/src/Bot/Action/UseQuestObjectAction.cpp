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

#include "UseQuestObjectAction.h"
#include "BotPlayerbotAI.h"
#include "CellImpl.h"
#include "GameObject.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "Log.h"
#include "MotionMaster.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "PlayerbotsConfig.h"
#include "QuestDef.h"
#include "SafeMovement.h"
#include "SharedDefines.h"
#include <unordered_set>

namespace
{
// Opportunistic-nearby reach for spotting the objective GO — wide enough that a bot arriving at the
// objective's quest POI (DoQuest travels it to the blob) sees the object, not so wide it detours to
// far-off spawns. Same "nearby, not routed" spirit as QuestGiverAction's search radius.
constexpr float QUEST_OBJECT_SEARCH_RADIUS = 60.0f;

// GO entries the bot still needs to *use* for an incomplete objective in its own quest log.
void CollectQuestObjectGoEntries(Player* bot, std::unordered_set<uint32>& entries)
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
            if (objective.Type != QUEST_OBJECTIVE_GAMEOBJECT)
                continue;

            if (objective.ObjectID <= 0)
                continue;

            if (!bot->IsQuestObjectiveComplete(slot, quest, objective))
                entries.insert(uint32(objective.ObjectID));
        }
    }
}

class QuestObjectGameObjectCheck
{
public:
    QuestObjectGameObjectCheck(Player* bot, float range, std::unordered_set<uint32> const* entries)
        : _bot(bot), _range(range), _entries(entries) { }

    bool operator()(GameObject* go) const
    {
        if (!_entries->contains(go->GetEntry()))
            return false;

        if (!go->isSpawned())
            return false;

        // Not ready to be used right now (already in use / activated / despawning).
        if (go->GetGoState() != GO_STATE_READY)
            return false;

        // Anti-ninja parity with AC's OpenLootAction GO guards — leave conditional / non-selectable
        // objects (e.g. instanced chests gated on an encounter) alone.
        if (go->HasFlag(GO_FLAG_INTERACT_COND))
            return false;

        if (go->HasFlag(GO_FLAG_NOT_SELECTABLE))
            return false;

        return _bot->IsWithinDist(go, _range);
    }

private:
    Player* _bot;
    float _range;
    std::unordered_set<uint32> const* _entries;
};
} // namespace

GameObject* UseQuestObjectAction::FindQuestObjectGameObject(Player* bot, float range)
{
    std::unordered_set<uint32> entries;
    CollectQuestObjectGoEntries(bot, entries);
    if (entries.empty())
        return nullptr;

    std::list<GameObject*> gameObjects;
    QuestObjectGameObjectCheck check(bot, range, &entries);
    Trinity::GameObjectListSearcher<QuestObjectGameObjectCheck> searcher(bot, gameObjects, check);
    Cell::VisitAllObjects(bot, searcher, range);

    GameObject* result = nullptr;
    float bestDistSq = (range * range) + 1.0f;
    for (GameObject* go : gameObjects)
    {
        float const distSq = bot->GetExactDistSq(go);
        if (distSq < bestDistSq)
        {
            bestDistSq = distSq;
            result = go;
        }
    }

    return result;
}

bool UseQuestObjectAction::IsUseful()
{
    Player* bot = GetBot();
    if (!bot || !bot->IsInWorld() || !bot->IsAlive() || bot->IsInCombat())
        return false;

    return FindQuestObjectGameObject(bot, QUEST_OBJECT_SEARCH_RADIUS) != nullptr;
}

bool UseQuestObjectAction::Execute(Event /*event*/)
{
    Player* bot = GetBot();
    if (!bot)
        return false;

    GameObject* go = FindQuestObjectGameObject(bot, QUEST_OBJECT_SEARCH_RADIUS);
    if (!go)
        return false;

    // Walk into interaction range if needed (SafeMovement contract — validated navmesh route only,
    // same as Wander/Grind/QuestGiver/Loot).
    if (bot->GetDistance(go) > Playerbots::GetQuestObjectUseDistance())
    {
        if (bot->GetMotionMaster()->GetCurrentMovementGeneratorType() == POINT_MOTION_TYPE)
            return false; // already walking somewhere this tick

        return TryMoveToValidatedPoint(bot, go->GetPositionX(), go->GetPositionY(), go->GetPositionZ());
    }

    // Replay the exact server-side gate the client's use opcode goes through
    // (WorldSession::HandleGameObjectUseOpcode → GetGameObjectIfCanInteractWith → GameObject::Use).
    GameObject* interactable = bot->GetGameObjectIfCanInteractWith(go->GetGUID());
    if (!interactable)
    {
        // Core says we're not quite in range/LOS yet — close the last bit of distance.
        return TryMoveToValidatedPoint(bot, go->GetPositionX(), go->GetPositionY(), go->GetPositionZ());
    }

    interactable->Use(bot);
    _botAI->GetRpgStatistics().objectsUsed++;
    TC_LOG_DEBUG("playerbots", "[New RPG] {} used quest object {} (entry {})",
        bot->GetName(), interactable->GetName(), interactable->GetEntry());

    return true;
}
