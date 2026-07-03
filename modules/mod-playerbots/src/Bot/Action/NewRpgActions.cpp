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

#include "NewRpgActions.h"
#include "BotPlayerbotAI.h"
#include "Log.h"
#include "Player.h"
#include "PlayerbotsConfig.h"
#include "QuestDef.h"
#include "Random.h"
#include "Timer.h"

// AC reference: mod-playerbots-master/src/Ai/World/Rpg/Action/NewRpgAction.cpp — same
// transition rules and DoQuest flow, TC-native quest APIs (typed objectives, ContentTuning-aware
// levels) and the SafeMovement contract underneath.

bool NewRpgStatusUpdateAction::Execute(Event /*event*/)
{
    Player* bot = GetBot();
    if (!bot || !bot->IsInWorld() || bot->IsInCombat())
        return false;

    NewRpgInfo& info = _botAI->GetRpgInfo();
    switch (info.GetStatus())
    {
        case RPG_IDLE:
            // Gate 10b subset of AC's candidate list (no camp/npc/rest/flight/pvp yet).
            return RandomChangeStatus({ RPG_GO_GRIND, RPG_WANDER_RANDOM, RPG_DO_QUEST });
        case RPG_GO_GRIND:
        {
            auto const& data = std::get<NewRpgInfo::GoGrind>(info.data);
            // GO_GRIND -> WANDER_RANDOM on arrival (AC: < 10yd from the chosen spot)
            if (bot->GetExactDist(data.pos) < 10.0f)
            {
                info.ChangeToWanderRandom();
                return true;
            }
            break;
        }
        case RPG_WANDER_RANDOM:
            // WANDER_RANDOM -> IDLE after the status duration
            if (info.HasStatusPersisted(Playerbots::GetRpgStatusWanderRandomDurationMs()))
            {
                info.ChangeToIdle();
                return true;
            }
            break;
        case RPG_DO_QUEST:
            // DO_QUEST -> IDLE after the status duration
            if (info.HasStatusPersisted(Playerbots::GetRpgStatusDoQuestDurationMs()))
            {
                info.ChangeToIdle();
                return true;
            }
            break;
        default:
            break;
    }

    return false;
}

bool NewRpgGoGrindAction::IsUseful()
{
    Player* bot = GetBot();
    return bot && bot->IsInWorld() && bot->IsAlive() && !bot->IsInCombat();
}

bool NewRpgGoGrindAction::Execute(Event /*event*/)
{
    if (auto* data = std::get_if<NewRpgInfo::GoGrind>(&_botAI->GetRpgInfo().data))
    {
        if (MoveFarTo(data->pos))
            return true;

        // Small nudge so the next tick's MoveFarTo starts from a slightly different position
        // (AC keeps it small so it doesn't look like the bot abandoned its destination).
        return MoveRandomNear(10.0f);
    }

    return false;
}

bool NewRpgDoQuestAction::IsUseful()
{
    Player* bot = GetBot();
    return bot && bot->IsInWorld() && bot->IsAlive() && !bot->IsInCombat();
}

bool NewRpgDoQuestAction::Execute(Event /*event*/)
{
    Player* bot = GetBot();
    if (!bot)
        return false;

    NewRpgInfo& info = _botAI->GetRpgInfo();
    auto* dataPtr = std::get_if<NewRpgInfo::DoQuest>(&info.data);
    if (!dataPtr)
        return false;

    switch (bot->GetQuestStatus(dataPtr->questId))
    {
        case QUEST_STATUS_INCOMPLETE:
            return DoIncompleteQuest(*dataPtr);
        case QUEST_STATUS_COMPLETE:
            return DoCompletedQuest(*dataPtr);
        default:
            break;
    }

    // Rewarded/failed/none — this quest is finished as a goal either way.
    info.ChangeToIdle();
    return true;
}

bool NewRpgDoQuestAction::DoIncompleteQuest(NewRpgInfo::DoQuest& data)
{
    Player* bot = GetBot();
    uint32 const questId = data.questId;

    // The pursued objective got completed — clear it so a new one gets resolved below
    // (AC checks the WotLK counter arrays; typed IsQuestObjectiveComplete replaces that).
    if (data.hasPos && data.objectiveId && bot->IsQuestObjectiveComplete(questId, data.objectiveId))
    {
        data.lastReachPOI = 0;
        data.hasPos = false;
        data.objectiveId = 0;
    }

    if (!data.hasPos)
    {
        std::vector<POIInfo> poiInfo;
        if (!GetQuestPOIPosAndObjective(questId, poiInfo))
        {
            // no POI travel target resolvable — stop pursuing this quest for now
            _botAI->GetRpgInfo().ChangeToIdle();
            return true;
        }

        POIInfo const& poi = poiInfo[urand(0, uint32(poiInfo.size()) - 1)];
        data.lastReachPOI = 0;
        data.pos = poi.pos;
        data.hasPos = true;
        data.objectiveId = poi.objectiveId;
    }

    if (bot->GetExactDist(data.pos) > 10.0f && !data.lastReachPOI)
    {
        if (MoveFarTo(data.pos))
            return true;

        // Long-range travel couldn't commit a leg — nudge so the next tick retries from a
        // different position instead of sitting idle.
        return MoveRandomNear(10.0f);
    }

    // Now we are near the quest objective; killing is done by the always-on "attack anything"
    // action (AC delegates to its grind strategy the same way).
    if (!data.lastReachPOI)
    {
        data.lastReachPOI = getMSTime();
        return true;
    }

    if (GetMSTimeDiffToNow(data.lastReachPOI) >= Playerbots::GetRpgPoiStayTimeMs())
    {
        // Stayed the full window — with zero progress on the pursued objective the quest is
        // probably not completable here (unkillable/ungatherable for this bot): abandon-set it.
        bool const hasProgression = data.objectiveId && bot->GetQuestObjectiveData(questId, data.objectiveId) != 0;
        if (!hasProgression)
        {
            _botAI->GetLowPriorityQuests().insert(questId);
            _botAI->GetRpgStatistics().questAbandoned++;
            TC_LOG_DEBUG("playerbots", "[New RPG] {} marked as abandoned quest {}", bot->GetName(), questId);
            _botAI->GetRpgInfo().ChangeToIdle();
            return true;
        }

        // progress happened, just slowly — clear and re-resolve a (possibly different) POI
        data.lastReachPOI = 0;
        data.hasPos = false;
        data.objectiveId = 0;
        return true;
    }

    // At the POI: small ~8yd wanders read as the bot looking around while kills/collections
    // progress (AC keeps the same small step to avoid pacing back and forth).
    return MoveRandomNear(8.0f);
}

bool NewRpgDoQuestAction::DoCompletedQuest(NewRpgInfo::DoQuest& data)
{
    Player* bot = GetBot();
    uint32 const questId = data.questId;

    if (!data.travelToTurnIn)
    {
        // Just completed while pursuing objectives — resolve the turn-in blob and head there.
        _botAI->GetRpgStatistics().questCompleted++;

        std::vector<POIInfo> poiInfo;
        if (!GetQuestPOIPosAndObjective(questId, poiInfo, true))
        {
            // no turn-in POI on this map/zone — stop pursuing, quest stays complete in the log
            _botAI->GetRpgInfo().ChangeToIdle();
            return false;
        }

        data.lastReachPOI = 0;
        data.pos = poiInfo[0].pos;
        data.hasPos = true;
        data.objectiveId = 0;
        data.travelToTurnIn = true;
    }

    if (!data.hasPos)
        return false;

    if (bot->GetExactDist(data.pos) > 10.0f && !data.lastReachPOI)
    {
        if (MoveFarTo(data.pos))
            return true;

        return MoveRandomNear(10.0f);
    }

    // Near the turn-in area — the higher-relevance "quest giver" action performs the actual
    // reward interaction (AC's SearchQuestGiverAndAcceptOrReward equivalent).
    if (!data.lastReachPOI)
    {
        data.lastReachPOI = getMSTime();
        return true;
    }

    if (GetMSTimeDiffToNow(data.lastReachPOI) >= Playerbots::GetRpgPoiStayTimeMs())
    {
        // e.g. the quest ender is a gameobject or otherwise not interactable for the bot
        _botAI->GetLowPriorityQuests().insert(questId);
        _botAI->GetRpgStatistics().questAbandoned++;
        TC_LOG_DEBUG("playerbots", "[New RPG] {} marked as abandoned quest {}", bot->GetName(), questId);
        _botAI->GetRpgInfo().ChangeToIdle();
        return true;
    }

    return false;
}
