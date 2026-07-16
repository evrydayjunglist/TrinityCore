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
#include "BotMapResidency.h"
#include "BotPlayerbotAI.h"
#include "Log.h"
#include "ObjectAccessor.h"
#include "ObjectDefines.h"
#include "Player.h"
#include "PlayerbotsConfig.h"
#include "QuestDef.h"
#include "Random.h"
#include "Timer.h"

namespace
{
// How close the bot walks up to a hub NPC before it counts as "reached" and starts its dwell (AC:
// IsWithinInteractionDist). Quest givers are accepted long before this by the always-on
// QuestGiverAction (relevance 30) within its own 80yd radius, so this distance only shapes the
// lifelike mingling among non-giver NPCs.
constexpr float WANDER_NPC_INTERACT_DIST = INTERACTION_DISTANCE;
}

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
            // AC's IDLE candidate set minus TravelFlight/OutdoorPvP (out of scope): GO_CAMP travels
            // to a distant NPC hub, WANDER_NPC mingles locally, REST sits. DO_QUEST stays dominant
            // via its weight so bots primarily quest rather than loiter.
            return RandomChangeStatus({ RPG_GO_CAMP, RPG_GO_GRIND, RPG_WANDER_RANDOM, RPG_DO_QUEST,
                                        RPG_WANDER_NPC, RPG_REST });
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
        case RPG_GO_CAMP:
        {
            auto const& data = std::get<NewRpgInfo::GoCamp>(info.data);
            // GO_CAMP -> WANDER_NPC on arrival at the hub centroid (AC: < 10yd), then mingle there.
            if (bot->GetExactDist(data.pos) < 10.0f)
            {
                info.ChangeToWanderNpc();
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
        case RPG_WANDER_NPC:
            // WANDER_NPC -> IDLE after its own duration (AC's statusWanderNpcDuration, 5 min). The
            // action also re-rolls to Idle when nothing's left worth visiting; this caps the
            // overall mingle session.
            if (info.HasStatusPersisted(Playerbots::GetRpgStatusWanderNpcDurationMs()))
            {
                info.ChangeToIdle();
                return true;
            }
            break;
        case RPG_REST:
            // REST -> IDLE after the rest duration (AC's statusRestDuration, 30s). No SetStandState
            // needed here — the next status' movement stands the bot automatically (AC parity).
            if (info.HasStatusPersisted(Playerbots::GetRpgStatusRestDurationMs()))
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
        Player* bot = GetBot();
        // Class 3: cold dest — drop the trip rather than nudge forever at the warm frontier.
        if (!IsBotMapPosQueryable(bot, data->pos))
        {
            TC_LOG_DEBUG("playerbots", "[New RPG] {} idle out of GO_GRIND — dest grid not resident",
                bot ? bot->GetName() : "?");
            _botAI->GetRpgInfo().ChangeToIdle();
            return true;
        }

        if (MoveFarTo(data->pos))
            return true;

        // Small nudge so the next tick's MoveFarTo starts from a slightly different position
        // (AC keeps it small so it doesn't look like the bot abandoned its destination).
        return MoveRandomNear(10.0f);
    }

    return false;
}

bool NewRpgGoCampAction::IsUseful()
{
    Player* bot = GetBot();
    return bot && bot->IsInWorld() && bot->IsAlive() && !bot->IsInCombat();
}

bool NewRpgGoCampAction::Execute(Event /*event*/)
{
    if (auto* data = std::get_if<NewRpgInfo::GoCamp>(&_botAI->GetRpgInfo().data))
    {
        Player* bot = GetBot();
        // Class 3: cold dest — drop the trip rather than nudge forever at the warm frontier.
        if (!IsBotMapPosQueryable(bot, data->pos))
        {
            TC_LOG_DEBUG("playerbots", "[New RPG] {} idle out of GO_CAMP — dest grid not resident",
                bot ? bot->GetName() : "?");
            _botAI->GetRpgInfo().ChangeToIdle();
            return true;
        }

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
        data.lastSweep = 0;
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
        data.lastSweep = 0;
        data.pos = poi.pos;
        data.hasPos = true;
        data.objectiveId = poi.objectiveId;
    }

    if (bot->GetExactDist(data.pos) > 10.0f && !data.lastReachPOI)
    {
        // Class 3: cold POI — clear and let status update pick another objective/quest rather
        // than pathfind across an unloaded frontier.
        if (!IsBotMapPosQueryable(bot, data.pos))
        {
            TC_LOG_DEBUG("playerbots", "[New RPG] {} clear cold quest POI ({},{},{}) — grid not resident",
                bot->GetName(), data.pos.GetPositionX(), data.pos.GetPositionY(), data.pos.GetPositionZ());
            data.hasPos = false;
            data.objectiveId = 0;
            data.lastReachPOI = 0;
            data.lastSweep = 0;
            return true;
        }

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
        data.lastSweep = 0;
        data.hasPos = false;
        data.objectiveId = 0;
        return true;
    }

    // Convergence F2 (playerbots-rpg-quest-convergence-fixes-handoff.md § 4-F2): zero progress so
    // far — don't circle one fixed point for the whole stay window; every sweep interval re-sample
    // a fresh random-weighted interior point of the same objective blob and walk there, so the bot
    // actually covers the objective area. That is what lets it *discover* stealthed objective mobs
    // the retail-like way (patrol until a frontal detect-range encounter happens naturally — the
    // core's stealth-detection gates are untouched). lastReachPOI is deliberately NOT reset: the
    // zero-progress abandon budget keeps running across sweep legs, so an impossible quest still
    // retires on schedule.
    if (data.objectiveId && bot->GetQuestObjectiveData(questId, data.objectiveId) == 0)
    {
        uint32 const lastLeg = data.lastSweep ? data.lastSweep : data.lastReachPOI;
        if (GetMSTimeDiffToNow(lastLeg) >= Playerbots::GetRpgPoiSweepIntervalMs())
        {
            // Each GetQuestPOIPosAndObjective call rolls fresh random weights per blob — keep only
            // the pursued objective's entries so the sweep stays inside the same objective area.
            std::vector<POIInfo> poiInfo;
            if (GetQuestPOIPosAndObjective(questId, poiInfo))
            {
                std::vector<POIInfo const*> sameObjective;
                for (POIInfo const& poi : poiInfo)
                    if (poi.objectiveId == data.objectiveId)
                        sameObjective.push_back(&poi);

                if (!sameObjective.empty())
                {
                    data.pos = sameObjective[urand(0, uint32(sameObjective.size()) - 1)]->pos;
                    data.lastSweep = getMSTime();
                    TC_LOG_DEBUG("playerbots", "[New RPG] {} sweeping to fresh POI point ({},{},{}) for quest {} objective {}",
                        bot->GetName(), data.pos.GetPositionX(), data.pos.GetPositionY(), data.pos.GetPositionZ(),
                        questId, data.objectiveId);
                }
            }
        }

        // Walking a committed sweep leg — same SafeMovement MoveFarTo path as the initial travel.
        if (data.lastSweep && bot->GetExactDist(data.pos) > 10.0f && MoveFarTo(data.pos))
            return true;
    }

    // At the POI: small ~8yd wanders read as the bot looking around while kills/collections
    // progress (AC keeps the same small step to avoid pacing back and forth); between F2 sweep
    // legs they also provide the frontal-arc turning stealth detection needs.
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
        // Class 3: cold turn-in POI — stop pursuing rather than stampede unloaded grids.
        if (!IsBotMapPosQueryable(bot, data.pos))
        {
            TC_LOG_DEBUG("playerbots", "[New RPG] {} idle out of quest turn-in — dest grid not resident",
                bot->GetName());
            _botAI->GetRpgInfo().ChangeToIdle();
            return true;
        }

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

bool NewRpgWanderNpcAction::IsUseful()
{
    Player* bot = GetBot();
    return bot && bot->IsInWorld() && bot->IsAlive() && !bot->IsInCombat();
}

bool NewRpgWanderNpcAction::Execute(Event /*event*/)
{
    Player* bot = GetBot();
    if (!bot)
        return false;

    NewRpgInfo& info = _botAI->GetRpgInfo();
    auto* dataPtr = std::get_if<NewRpgInfo::WanderNpc>(&info.data);
    if (!dataPtr)
        return false;

    NewRpgInfo::WanderNpc& data = *dataPtr;

    // Need a target (fresh entry, or the previous one was dwelt/cleared): pick the next NPC to
    // visit — an actionable giver first, else a not-recently-visited hub NPC. Nothing left worth
    // visiting -> Idle so the status machine re-rolls (AC: ChooseNpcOrGameObjectToInteract empty).
    if (data.target.IsEmpty())
    {
        ObjectGuid next;
        if (!SelectRandomNpcToInteract(next, data.visited))
        {
            info.ChangeToIdle();
            return true;
        }

        data.target = next;
        data.lastReach = 0;
        return true;
    }

    // Re-resolve each tick (AC re-fetches via ObjectAccessor — the target may have despawned or
    // moved out of the grid). Gone -> mark visited, drop the target, pick the next one next tick.
    WorldObject* target = ObjectAccessor::GetWorldObject(*bot, data.target);
    if (!target || !target->IsInWorld())
    {
        data.visited.insert(data.target);
        data.target = ObjectGuid();
        data.lastReach = 0;
        return true;
    }

    // Still walking up to it.
    if (bot->GetExactDist(*target) > WANDER_NPC_INTERACT_DIST)
    {
        data.lastReach = 0;
        if (MoveFarTo(target->GetPosition()))
            return true;

        // Pathing couldn't commit a leg (NPC in a wall, mmap hiccup) — small step so the next tick
        // retries from a different spot instead of staring at the NPC (AC's MoveRandomNear(15)).
        return MoveRandomNear(15.0f);
    }

    // Reached it. Quest givers are accepted by the always-on QuestGiverAction (relevance 30) — this
    // action just dwells to read as the bot lingering at the NPC (AC's npcStayTime), then cycles on.
    if (!data.lastReach)
    {
        data.lastReach = getMSTime();
        return true;
    }

    if (GetMSTimeDiffToNow(data.lastReach) < Playerbots::GetRpgWanderNpcStayTimeMs())
        return false; // dwell — let lower-relevance behaviour (or nothing) run this tick

    // Dwelt long enough — move on to the next hub NPC.
    data.visited.insert(data.target);
    data.target = ObjectGuid();
    data.lastReach = 0;
    return true;
}
