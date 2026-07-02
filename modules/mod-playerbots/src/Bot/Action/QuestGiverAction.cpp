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

#include "QuestGiverAction.h"
#include "CellImpl.h"
#include "GameObject.h"
#include "GossipDef.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "LootItemType.h"
#include "MotionMaster.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "QuestDef.h"

namespace
{
constexpr float QUEST_GIVER_SEARCH_RADIUS = 30.0f;

class QuestGiverCreatureCheck
{
public:
    QuestGiverCreatureCheck(WorldObject const* obj, float range) : _obj(obj), _range(range) { }

    bool operator()(Creature* creature) const
    {
        return creature->IsAlive() && creature->IsQuestGiver() && _obj->IsWithinDist(creature, _range);
    }

private:
    WorldObject const* _obj;
    float _range;
};

class QuestGiverGameObjectCheck
{
public:
    QuestGiverGameObjectCheck(WorldObject const* obj, float range) : _obj(obj), _range(range) { }

    bool operator()(GameObject* go) const
    {
        return go->isSpawned() && go->GetGoType() == GAMEOBJECT_TYPE_QUESTGIVER && _obj->IsWithinDist(go, _range);
    }

private:
    WorldObject const* _obj;
    float _range;
};

// Rebuilds the bot's quest menu for this object (same call the client's gossip window
// triggers) and reports whether anything is actually offerable/turn-in-able right now.
bool HasActionableQuest(Player* bot, WorldObject* questGiver)
{
    bot->PrepareQuestMenu(questGiver->GetGUID());
    return !bot->PlayerTalkClass->GetQuestMenu().Empty();
}

WorldObject* FindNearbyQuestGiver(Player* bot)
{
    std::list<Creature*> creatures;
    QuestGiverCreatureCheck creatureCheck(bot, QUEST_GIVER_SEARCH_RADIUS);
    Trinity::CreatureListSearcher<QuestGiverCreatureCheck> creatureSearcher(bot, creatures, creatureCheck);
    Cell::VisitAllObjects(bot, creatureSearcher, QUEST_GIVER_SEARCH_RADIUS);

    std::list<GameObject*> gameObjects;
    QuestGiverGameObjectCheck goCheck(bot, QUEST_GIVER_SEARCH_RADIUS);
    Trinity::GameObjectListSearcher<QuestGiverGameObjectCheck> goSearcher(bot, gameObjects, goCheck);
    Cell::VisitAllObjects(bot, goSearcher, QUEST_GIVER_SEARCH_RADIUS);

    WorldObject* best = nullptr;
    float bestDistSq = (QUEST_GIVER_SEARCH_RADIUS * QUEST_GIVER_SEARCH_RADIUS) + 1.0f;

    auto consider = [&](WorldObject* candidate)
    {
        if (!HasActionableQuest(bot, candidate))
            return;

        float const distSq = bot->GetExactDistSq(*candidate);
        if (distSq < bestDistSq)
        {
            bestDistSq = distSq;
            best = candidate;
        }
    };

    for (Creature* creature : creatures)
        consider(creature);

    for (GameObject* go : gameObjects)
        consider(go);

    return best;
}

// Accepts every offerable quest and turns in every completed one at this quest giver,
// replaying the same server-side checks the client's accept/choose-reward flow uses
// (WorldSession::HandleQuestgiverAcceptQuestOpcode / HandleQuestgiverChooseRewardOpcode)
// without a UI in the loop. No hardcoded quest id — driven entirely by what the world DB
// actually offers this bot at this object.
bool InteractWithQuestGiver(Player* bot, WorldObject* questGiver)
{
    bot->PrepareQuestMenu(questGiver->GetGUID());
    QuestMenu& menu = bot->PlayerTalkClass->GetQuestMenu();

    bool didSomething = false;
    for (uint8 i = 0; i < menu.GetMenuItemCount(); ++i)
    {
        QuestMenuItem const& item = menu.GetItem(i);
        Quest const* quest = sObjectMgr->GetQuestTemplate(item.QuestId);
        if (!quest)
            continue;

        if (bot->GetQuestStatus(item.QuestId) == QUEST_STATUS_COMPLETE)
        {
            if (!bot->CanRewardQuest(quest, false))
                continue;

            // Bots have no UI to pick among alternative rewards — just take the first choice
            // slot (same field order the client's default-selected radio button would show).
            bool const hasChoice = quest->GetRewChoiceItemsCount() > 0;
            LootItemType const rewardType = hasChoice ? quest->RewardChoiceItemType[0] : LootItemType::Item;
            uint32 const rewardItemId = hasChoice ? quest->RewardChoiceItemId[0] : 0;
            if (!bot->CanRewardQuest(quest, rewardType, rewardItemId, false))
                continue;

            bot->RewardQuest(quest, rewardType, rewardItemId, questGiver);
            didSomething = true;
        }
        else if (bot->GetQuestStatus(item.QuestId) == QUEST_STATUS_NONE)
        {
            if (!bot->CanTakeQuest(quest, false) || !bot->CanAddQuest(quest, false))
                continue;

            bot->AddQuestAndCheckCompletion(quest, questGiver);
            didSomething = true;
        }
    }

    return didSomething;
}
} // namespace

bool QuestGiverAction::IsUseful()
{
    Player* bot = GetBot();
    if (!bot || !bot->IsInWorld() || bot->IsInCombat())
        return false;

    return FindNearbyQuestGiver(bot) != nullptr;
}

bool QuestGiverAction::Execute(Event /*event*/)
{
    Player* bot = GetBot();
    if (!bot)
        return false;

    WorldObject* questGiver = FindNearbyQuestGiver(bot);
    if (!questGiver)
        return false;

    if (!bot->CanInteractWithQuestGiver(questGiver))
    {
        if (bot->GetMotionMaster()->GetCurrentMovementGeneratorType() == POINT_MOTION_TYPE)
            return false; // already walking toward a destination this tick

        bot->GetMotionMaster()->MovePoint(0, questGiver->GetPositionX(), questGiver->GetPositionY(), questGiver->GetPositionZ());
        return true;
    }

    return InteractWithQuestGiver(bot, questGiver);
}
