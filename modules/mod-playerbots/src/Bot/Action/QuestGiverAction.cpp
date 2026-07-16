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
#include "BotPlayerbotAI.h"
#include "CellImpl.h"
#include "GameObject.h"
#include "GossipDef.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "Log.h"
#include "LootItemType.h"
#include "MotionMaster.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "QuestDef.h"

namespace
{
// Gate 10b: widened from 30yd to AC's SearchQuestGiverAndAcceptOrReward radius (80yd) — still
// opportunistic-nearby pickup only, no distant quest-giver routing (handoff scope).
constexpr float QUEST_GIVER_SEARCH_RADIUS = 80.0f;

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

} // namespace

// Convergence fix F1 (playerbots-rpg-quest-convergence-fixes-handoff.md § 4-F1): the per-candidate
// gate is the strict HasQuestToAcceptOrReward — a COMPLETE quest the bot can be rewarded for, or a
// NONE quest it can take that's worth + capable of doing. The previous menu-non-empty gate was the
// giver-anchor ping-pong bug: TC quest menus include turn-in quests the player holds INCOMPLETE
// (the greyed "come back later" entry, Player::PrepareQuestMenu), so an INCOMPLETE-only ender kept
// this action permanently useful while InteractWithQuestGiver could never transact anything —
// hijacking every idle tick into a walk toward the ender. AC parity: ChooseNpcOrGameObjectToInteract
// only ever targets candidates passing CanInteractWithQuestGiver && HasQuestToAcceptOrReward.
WorldObject* QuestGiverAction::FindNearbyQuestGiver()
{
    Player* bot = GetBot();
    if (!bot)
        return nullptr;

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
        if (!HasQuestToAcceptOrReward(candidate))
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
// actually offers this bot at this object. Gate 10b: accepts additionally pass AC's
// IsQuestWorthDoing/IsQuestCapableDoing filters (InteractWithNpcOrGameObjectForQuest parity)
// and feed the RPG statistics counters.
bool QuestGiverAction::InteractWithQuestGiver(WorldObject* questGiver)
{
    Player* bot = GetBot();
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
            _botAI->GetRpgStatistics().questRewarded++;
            TC_LOG_DEBUG("playerbots", "[New RPG] {} rewarded quest {}", bot->GetName(), item.QuestId);
            didSomething = true;
        }
        else if (bot->GetQuestStatus(item.QuestId) == QUEST_STATUS_NONE)
        {
            if (!bot->CanTakeQuest(quest, false) || !bot->CanAddQuest(quest, false))
                continue;

            if (!IsQuestWorthDoing(quest) || !IsQuestCapableDoing(quest))
                continue;

            bot->AddQuestAndCheckCompletion(quest, questGiver);
            _botAI->GetRpgStatistics().questAccepted++;
            TC_LOG_DEBUG("playerbots", "[New RPG] {} accepted quest {}", bot->GetName(), item.QuestId);
            didSomething = true;
        }
    }

    return didSomething;
}

bool QuestGiverAction::IsUseful()
{
    Player* bot = GetBot();
    if (!bot || !bot->IsInWorld() || bot->IsInCombat())
        return false;

    return FindNearbyQuestGiver() != nullptr;
}

bool QuestGiverAction::Execute(Event /*event*/)
{
    Player* bot = GetBot();
    if (!bot)
        return false;

    // Quest log hygiene before any new pickup (AC runs OrganizeQuestLog at the top of
    // SearchQuestGiverAndAcceptOrReward) — drops greyed-out/incapable/failed quests when
    // the log is nearly full so worthwhile ones still fit.
    OrganizeQuestLog();

    WorldObject* questGiver = FindNearbyQuestGiver();
    if (!questGiver)
        return false;

    if (!bot->CanInteractWithQuestGiver(questGiver))
    {
        if (bot->GetMotionMaster()->GetCurrentMovementGeneratorType() == POINT_MOTION_TYPE)
            return false; // already walking toward a destination this tick

        // Approach via MoveFarTo (obstacle-pathing handoff §5-D2) rather than a bare
        // TryMoveToValidatedPoint: this was the one raw, fallback-less locomotion call site, so a
        // failed direct attempt used to leave the bot standing still at an obstacle that tick.
        // Going through MoveFarTo inherits the same SafeMovement validation plus the D1 progress
        // guard, the D2 leg-scaled forward-cone stepping-stone fallback, and the shared stuck
        // accounting every other locomotion site already uses.
        return MoveFarTo(questGiver->GetPosition());
    }

    return InteractWithQuestGiver(questGiver);
}
