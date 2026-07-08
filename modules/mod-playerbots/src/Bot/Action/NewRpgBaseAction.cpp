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

#include "NewRpgBaseAction.h"
#include "AttackValidity.h"
#include "Bot/Rpg/GrindLocationCache.h"
#include "Bot/Rpg/HubLocationCache.h"
#include "BotPlayerbotAI.h"
#include "CellImpl.h"
#include "GameObject.h"
#include "GossipDef.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "Log.h"
#include "Map.h"
#include "MotionMaster.h"
#include "MoveSpline.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "PlayerbotsConfig.h"
#include "QuestDef.h"
#include "Random.h"
#include "SafeMovement.h"
#include "ScriptMgr.h"
#include "Timer.h"
#include "UnitDefines.h"
#include "World.h"
#include <algorithm>
#include <cmath>

namespace
{
// AC: NewRpgBaseAction.cpp GenerateRandomWeights — a random convex combination over a blob's
// points, so repeated POI picks land at varied spots inside the objective area instead of
// always the same centroid.
std::vector<float> GenerateRandomWeights(size_t count)
{
    std::vector<float> weights(count);
    float sum = 0.0f;
    for (float& weight : weights)
    {
        weight = float(rand_norm()) + 0.01f;
        sum += weight;
    }
    for (float& weight : weights)
        weight /= sum;
    return weights;
}

// Same live-search radius Gate 10's grind behavior uses ("what to attack right now").
constexpr float RPG_NEARBY_HOSTILE_RADIUS = 30.0f;

// Gate 10 grind-spot level gating, moved here with the spot selection (was GrindAction.cpp).
constexpr uint32 GRIND_LEVEL_TOLERANCE = 5;

bool IsSpotForBotLevel(Player* bot, GrindSpot const& spot)
{
    // Scaling-aware spots (ContentTuningID != 0) are viable for any bot level in range —
    // don't force them into a static AC-style level bucket (Gate 10 handoff § TC-Midnight
    // adaptations).
    if (spot.ScalingAware)
        return true;

    uint32 const level = bot->GetLevel();
    return level + GRIND_LEVEL_TOLERANCE >= spot.MinLevel && level <= spot.MaxLevel + GRIND_LEVEL_TOLERANCE;
}

// AC's PossibleRpgTargetsValue::allowedNpcFlags — the service-NPC set a bot will walk up to and
// mingle among in WANDER_NPC (innkeeper / gossip / questgiver / flightmaster / banker / trainer /
// vendor / …). All fit in the low 32 bits of the npcflag field, so one combined mask + a single
// HasNpcFlag(mask) call (true if ANY bit matches) reproduces AC's per-flag loop.
constexpr NPCFlags HUB_NPC_ALLOWED_MASK = NPCFlags(
    UNIT_NPC_FLAG_INNKEEPER | UNIT_NPC_FLAG_GOSSIP | UNIT_NPC_FLAG_QUESTGIVER |
    UNIT_NPC_FLAG_FLIGHTMASTER | UNIT_NPC_FLAG_BANKER | UNIT_NPC_FLAG_GUILD_BANKER |
    UNIT_NPC_FLAG_TRAINER_CLASS | UNIT_NPC_FLAG_TRAINER_PROFESSION |
    UNIT_NPC_FLAG_VENDOR_AMMO | UNIT_NPC_FLAG_VENDOR_FOOD | UNIT_NPC_FLAG_VENDOR_POISON |
    UNIT_NPC_FLAG_VENDOR_REAGENT | UNIT_NPC_FLAG_AUCTIONEER | UNIT_NPC_FLAG_STABLEMASTER |
    UNIT_NPC_FLAG_PETITIONER | UNIT_NPC_FLAG_TABARDDESIGNER | UNIT_NPC_FLAG_BATTLEMASTER |
    UNIT_NPC_FLAG_TRAINER | UNIT_NPC_FLAG_VENDOR | UNIT_NPC_FLAG_REPAIR);

// WANDER_NPC scan grid checks — alive allowed-flag service NPC / spawned GAMEOBJECT_TYPE_QUESTGIVER
// GO in range. Broader than the seeking slice's quest-giver-only check (givers are a subset of the
// mask); hostile/spirit-healer/player exclusion (AC's AcceptUnit) is applied per-candidate in the scan.
class WanderNpcCreatureCheck
{
public:
    WanderNpcCreatureCheck(WorldObject const* obj, float range) : _obj(obj), _range(range) { }

    bool operator()(Creature* creature) const
    {
        return creature->IsAlive() && creature->HasNpcFlag(HUB_NPC_ALLOWED_MASK)
            && _obj->IsWithinDist(creature, _range);
    }

private:
    WorldObject const* _obj;
    float _range;
};

class SeekQuestGiverGameObjectCheck
{
public:
    SeekQuestGiverGameObjectCheck(WorldObject const* obj, float range) : _obj(obj), _range(range) { }

    bool operator()(GameObject* go) const
    {
        return go->isSpawned() && go->GetGoType() == GAMEOBJECT_TYPE_QUESTGIVER && _obj->IsWithinDist(go, _range);
    }

private:
    WorldObject const* _obj;
    float _range;
};
}

bool NewRpgBaseAction::MoveFarTo(Position const& dest)
{
    Player* bot = GetBot();
    if (!bot)
        return false;

    NewRpgInfo& info = _botAI->GetRpgInfo();

    // Clear stuck tracking when the destination changes (AC: dest != rpgInfo.moveFarPos →
    // SetMoveFarTo). Exact float compare is fine — dest always comes from the same stored
    // status payload, not recomputed coordinates.
    if (!info.hasMoveFarPos || !(info.moveFarPos == dest))
        info.SetMoveFarTo(dest);

    // Let a previously committed long spline finish before recomputing (AC gates on "still
    // moving toward the last committed point with > 10yd left"): re-pathing mid-walk from a new
    // position often yields a different partial-route endpoint, and reissuing MovePoint every
    // tick makes the bot oscillate around obstacles instead of walking around them.
    if (!bot->movespline->Finalized())
    {
        G3D::Vector3 const end = bot->movespline->FinalDestination();
        if (bot->GetExactDist(end.x, end.y, end.z) > 10.0f)
            return true;
    }

    float const disToDest = bot->GetExactDist(dest);

    // Stuck tracking (AC: require a meaningful 5yd improvement to reset; 5 failed attempts over
    // the 90s window → teleport recovery so the bot gets on with its RPG objective instead of
    // oscillating forever). The teleport completes headlessly thanks to the bot-session teleport
    // self-ack fix (playerbots-bot-teleport-ack-handoff.md).
    if (disToDest + 5.0f < info.nearestMoveFarDis)
    {
        info.nearestMoveFarDis = disToDest;
        info.stuckTs = getMSTime();
        info.stuckAttempts = 0;
    }
    else if (++info.stuckAttempts >= 5 && info.stuckTs && GetMSTimeDiffToNow(info.stuckTs) >= STUCK_TIME_MS)
    {
        info.stuckTs = getMSTime();
        info.stuckAttempts = 0;
        TC_LOG_DEBUG("playerbots", "[New RPG] Teleport {} from ({},{},{}) to ({},{},{}) map {} as it is stuck moving far",
            bot->GetName(), bot->GetPositionX(), bot->GetPositionY(), bot->GetPositionZ(),
            dest.GetPositionX(), dest.GetPositionY(), dest.GetPositionZ(), bot->GetMapId());
        return bot->TeleportTo(bot->GetMapId(), dest.GetPositionX(), dest.GetPositionY(), dest.GetPositionZ(),
            bot->GetOrientation());
    }

    // Close enough for a single validated leg (AC: dis < pathFinderDis → plain MoveTo).
    if (disToDest < PATH_FINDER_DIS)
        return TryMoveToValidatedPoint(bot, dest.GetPositionX(), dest.GetPositionY(), dest.GetPositionZ());

    // Primary: route toward the true destination and walk the furthest reachable waypoint —
    // partial (PATHFIND_INCOMPLETE) routes past the ~296yd smooth-path cap are exactly what we
    // want here, as long as they actually close the gap (AC's "endDistToDest + 5.0f < disToDest").
    if (TryMoveTowardValidatedPoint(bot, dest.GetPositionX(), dest.GetPositionY(), dest.GetPositionZ(), 5.0f))
        return true;

    // Fallback: sample the forward cone for a reachable stepping stone (AC: 2 samples, ±π/2
    // around the bearing to dest, 0.5–1.0 × pathFinderDis, preferring the more-forward sample).
    float const baseAngle = bot->GetAbsoluteAngle(dest.GetPositionX(), dest.GetPositionY());
    float deltas[2] = { (float(rand_norm()) - 0.5f) * float(M_PI), (float(rand_norm()) - 0.5f) * float(M_PI) };
    if (std::fabs(deltas[1]) < std::fabs(deltas[0]))
        std::swap(deltas[0], deltas[1]);

    for (float delta : deltas)
    {
        float const sampleDis = (0.5f + float(rand_norm()) * 0.5f) * PATH_FINDER_DIS;
        float const angle = baseAngle + delta;
        float const dx = bot->GetPositionX() + std::cos(angle) * sampleDis;
        float const dy = bot->GetPositionY() + std::sin(angle) * sampleDis;
        if (TryMoveToValidatedPoint(bot, dx, dy, bot->GetPositionZ() + 0.5f))
            return true;
    }

    return false;
}

bool NewRpgBaseAction::MoveRandomNear(float moveStep)
{
    Player* bot = GetBot();
    if (!bot)
        return false;

    if (!bot->movespline->Finalized())
        return false; // still walking a committed leg — don't clobber it

    float const x = bot->GetPositionX();
    float const y = bot->GetPositionY();
    float const z = bot->GetPositionZ();

    // AC: NewRpgBaseAction::MoveRandomNear — up to 8 samples so a single bad roll (water,
    // blocked geometry, unreachable poly) doesn't leave the bot standing still. Validation is
    // the SafeMovement contract instead of AC's map-extension checks.
    for (int attempt = 0; attempt < 8; ++attempt)
    {
        float const distance = (0.4f + float(rand_norm()) * 0.6f) * moveStep;
        float const angle = float(rand_norm()) * 2.0f * float(M_PI);
        float const dx = x + distance * std::cos(angle);
        float const dy = y + distance * std::sin(angle);
        if (TryMoveToValidatedPoint(bot, dx, dy, z))
            return true;
    }

    return false;
}

bool NewRpgBaseAction::IsQuestWorthDoing(Quest const* quest) const
{
    Player* bot = GetBot();
    if (!bot || !quest)
        return false;

    // E4 resolution (Gate 10b handoff §5): AC's WotLK formula shape, with the core's own
    // ContentTuning-aware Player::GetQuestLevel underneath — the exact computation TC itself
    // uses for the "trivial quest" (grey) determination in quest-giver status dialogs.
    bool const isLowLevelQuest = int32(bot->GetLevel()) >
        (bot->GetQuestLevel(quest) + int32(sWorld->getIntConfig(CONFIG_QUEST_LOW_LEVEL_HIDE_DIFF)));

    if (isLowLevelQuest)
        return false;

    if (quest->IsRepeatable())
        return false;

    if (quest->IsSeasonal())
        return false;

    return true;
}

bool NewRpgBaseAction::IsQuestCapableDoing(Quest const* quest) const
{
    Player* bot = GetBot();
    if (!bot || !quest)
        return false;

    // AC: bot->GetLevel() + 3 < bot->GetQuestLevel(quest). TC's GetQuestLevel clamps into the
    // ContentTuning band, returning the tuning MinLevel while the bot is below it — so this
    // rejects quests whose content floor is 4+ levels above the bot, same intent as AC.
    bool const highLevelQuest = int32(bot->GetLevel()) + 3 < bot->GetQuestLevel(quest);
    if (highLevelQuest)
        return false;

    // AC excludes elite/dungeon/raid via WotLK's Quest::GetType(); the modern equivalent is the
    // QuestInfo id (dungeon/raid/group/heroic/scenario...) — cover raids via the core helper and
    // group content via SuggestedPlayers, the two signals that exist on Midnight data.
    if (quest->IsRaidQuest(bot->GetMap()->GetDifficultyID()))
        return false;

    // now we are only capable of doing solo quests (AC keeps the same rule)
    if (quest->GetSuggestedPlayers() >= 2)
        return false;

    return true;
}

bool NewRpgBaseAction::HasQuestToAcceptOrReward(WorldObject* questGiver) const
{
    Player* bot = GetBot();
    if (!bot || !questGiver)
        return false;

    // Rebuild the bot's quest menu for this object (same call the client's gossip window makes).
    bot->PrepareQuestMenu(questGiver->GetGUID());
    QuestMenu const& menu = bot->PlayerTalkClass->GetQuestMenu();
    if (menu.Empty())
        return false;

    // AC parity (NewRpgBaseAction::HasQuestToAcceptOrReward): a completed quest we can be
    // rewarded for, or a fresh quest we can take that's worth + capable of doing.
    for (uint8 i = 0; i < menu.GetMenuItemCount(); ++i)
    {
        QuestMenuItem const& item = menu.GetItem(i);
        Quest const* quest = sObjectMgr->GetQuestTemplate(item.QuestId);
        if (!quest)
            continue;

        QuestStatus const status = bot->GetQuestStatus(item.QuestId);
        if (status == QUEST_STATUS_COMPLETE)
        {
            if (bot->CanRewardQuest(quest, false))
                return true;
        }
        else if (status == QUEST_STATUS_NONE)
        {
            if (bot->CanTakeQuest(quest, false) && bot->CanAddQuest(quest, false)
                && IsQuestWorthDoing(quest) && IsQuestCapableDoing(quest))
                return true;
        }
    }

    return false;
}

bool NewRpgBaseAction::IsQuestUnactionable(uint32 questId) const
{
    Player* bot = GetBot();
    if (!bot || !questId)
        return false;

    // Already classified this session — cheap re-check.
    if (_botAI->GetUnactionableQuests().contains(questId))
        return true;

    // Conservative V1 detection (handoff §4): only the provably-impossible case. A quest sitting
    // COMPLETE with no quest-ender anywhere in world data can never be turned in by ANY player, so
    // ignoring it cannot skip real content. INCOMPLETE-but-unachievable is NOT safely detectable
    // (cross-map objectives are valid) and is left to the POI-timeout → low-priority path.
    if (bot->GetQuestStatus(questId) != QUEST_STATUS_COMPLETE)
        return false;

    auto const creatureEnders = sObjectMgr->GetCreatureQuestInvolvedRelationReverseBounds(questId);
    if (creatureEnders.begin() != creatureEnders.end())
        return false;

    auto const goEnders = sObjectMgr->GetGOQuestInvolvedRelationReverseBounds(questId);
    if (goEnders.begin() != goEnders.end())
        return false;

    // No ender of either kind — record it once so selection/status treat it as ignorable. In
    // memory only; never DropQuest/AbandonQuest (owner directive).
    _botAI->GetUnactionableQuests().insert(questId);
    TC_LOG_DEBUG("playerbots", "[New RPG] {} ignoring unactionable (no-ender COMPLETE) quest {}",
        bot->GetName(), questId);
    return true;
}

void NewRpgBaseAction::ScanWanderNpcTargets(ObjectGuid& nearestGiverOut, std::vector<ObjectGuid>& hubNpcsOut)
{
    nearestGiverOut = ObjectGuid();
    hubNpcsOut.clear();

    Player* bot = GetBot();
    if (!bot)
        return;

    float const radius = Playerbots::GetRpgWanderNpcRadius();

    std::list<Creature*> creatures;
    WanderNpcCreatureCheck creatureCheck(bot, radius);
    Trinity::CreatureListSearcher<WanderNpcCreatureCheck> creatureSearcher(bot, creatures, creatureCheck);
    Cell::VisitAllObjects(bot, creatureSearcher, radius);

    std::list<GameObject*> gameObjects;
    SeekQuestGiverGameObjectCheck goCheck(bot, radius);
    Trinity::GameObjectListSearcher<SeekQuestGiverGameObjectCheck> goSearcher(bot, gameObjects, goCheck);
    Cell::VisitAllObjects(bot, goSearcher, radius);

    float bestGiverDistSq = (radius * radius) + 1.0f;
    std::vector<std::pair<float, ObjectGuid>> hubs; // (distSq, guid), sorted nearest-first below

    auto considerGiver = [&](WorldObject* candidate)
    {
        if (!HasQuestToAcceptOrReward(candidate))
            return;

        float const distSq = bot->GetExactDistSq(*candidate);
        if (distSq < bestGiverDistSq)
        {
            bestGiverDistSq = distSq;
            nearestGiverOut = candidate->GetGUID();
        }
    };

    for (Creature* creature : creatures)
    {
        // AC's AcceptUnit exclusions (spirit healers / hostiles); players can't reach this scan.
        if (creature->IsSpiritService() || bot->IsHostileTo(creature))
            continue;

        hubs.push_back({ bot->GetExactDistSq(*creature), creature->GetGUID() });

        // A quest giver with something to transact is the priority target (quest acquisition first).
        if (creature->IsQuestGiver())
            considerGiver(creature);
    }

    for (GameObject* go : gameObjects)
        considerGiver(go);

    std::sort(hubs.begin(), hubs.end(), [](auto const& a, auto const& b) { return a.first < b.first; });
    hubNpcsOut.reserve(hubs.size());
    for (auto const& [distSq, guid] : hubs)
        hubNpcsOut.push_back(guid);
}

bool NewRpgBaseAction::SelectRandomNpcToInteract(ObjectGuid& targetOut, std::unordered_set<ObjectGuid> const& visited)
{
    ObjectGuid nearestGiver;
    std::vector<ObjectGuid> hubNpcs;
    ScanWanderNpcTargets(nearestGiver, hubNpcs);

    // Quest acquisition stays the priority (handoff §4): if a giver with an actionable quest is in
    // range, target it first so the un-stranding behaviour is preserved. QuestGiverAction (rel 30)
    // does the actual accept/reward once the bot is within its 80yd radius.
    if (!nearestGiver.IsEmpty())
    {
        targetOut = nearestGiver;
        return true;
    }

    // Otherwise mingle — but only at a genuine hub (AC's ">= 3 NPCs" rule), and prefer one not
    // recently dwelt at so the bot cycles through the hub rather than ping-ponging the nearest.
    if (hubNpcs.size() < HUB_MIN_NPCS)
        return false;

    for (ObjectGuid const& guid : hubNpcs) // sorted nearest-first
    {
        if (!visited.contains(guid))
        {
            targetOut = guid;
            return true;
        }
    }

    // Every hub NPC visited this session — restart from the nearest so mingling continues until the
    // status duration expires (the visited set is cleared when WANDER_NPC is (re-)entered).
    targetOut = hubNpcs.front();
    return true;
}

bool NewRpgBaseAction::SelectRandomHubPos(Position& destOut)
{
    Player* bot = GetBot();
    if (!bot)
        return false;

    // AC: NewRpgBaseAction::SelectRandomCampPos — hubs from TravelMgr.GetTravelHubs; here the source
    // is HubLocationCache (built from sObjectMgr creature data the same way GrindLocationCache is).
    std::vector<HubSpot> const& spots = sHubLocationCache->GetSpotsForMap(bot->GetMapId());
    if (spots.empty())
        return false;

    float const range = bot->GetLevel() <= 5 ? 500.0f : 2500.0f;

    std::vector<HubSpot const*> pool;
    for (HubSpot const& spot : spots)
    {
        float const dist = bot->GetExactDist(spot.Pos);
        if (dist > range)
            continue;

        // Must actually travel (AC's camp rule): skip a hub the bot is already standing in — those
        // are handled by WANDER_NPC's local scan, not a GO_CAMP trip.
        if (dist < 50.0f)
            continue;

        pool.push_back(&spot);
    }

    if (pool.empty())
        return false;

    // Sample-and-check same-zone like SelectRandomGrindPos (Map::GetZoneId is a real terrain query,
    // so probe a handful rather than zone-filtering the whole pool up front).
    for (int attempt = 0; attempt < 5; ++attempt)
    {
        HubSpot const* spot = pool[urand(0, uint32(pool.size()) - 1)];
        if (bot->GetMap()->GetZoneId(bot->GetPhaseShift(), spot->Pos) != bot->GetZoneId())
            continue;

        destOut = spot->Pos;
        return true;
    }

    return false;
}

bool NewRpgBaseAction::OrganizeQuestLog()
{
    Player* bot = GetBot();
    if (!bot)
        return false;

    int32 freeSlotNum = 0;
    for (uint16 slot = 0; slot < MAX_QUEST_LOG_SIZE; ++slot)
        if (!bot->GetQuestSlotQuestId(slot))
            ++freeSlotNum;

    // it's ok if we have two or more free slots (AC keeps the same threshold)
    if (freeSlotNum >= 2)
        return false;

    int32 dropped = 0;

    // Pass 1 (AC parity): drop quests not worth doing / not capable of doing / failed.
    for (uint16 slot = 0; slot < MAX_QUEST_LOG_SIZE; ++slot)
    {
        uint32 const questId = bot->GetQuestSlotQuestId(slot);
        if (!questId)
            continue;

        Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
        if (!quest)
            continue;

        if (!IsQuestWorthDoing(quest) || !IsQuestCapableDoing(quest) ||
            bot->GetQuestStatus(questId) == QUEST_STATUS_FAILED)
        {
            DropQuest(questId);
            ++dropped;
        }
    }

    // dropping 8+ at once is enough breathing room — avoid repeated accept-and-drop churn (AC)
    if (dropped >= 8)
        return true;

    // Pass 2 (AC parity): drop festival/class-sort quests and quests belonging to other zones.
    for (uint16 slot = 0; slot < MAX_QUEST_LOG_SIZE; ++slot)
    {
        uint32 const questId = bot->GetQuestSlotQuestId(slot);
        if (!questId)
            continue;

        Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
        if (!quest)
            continue;

        int32 const zoneOrSort = quest->GetZoneOrSort();
        if (zoneOrSort < 0 || (zoneOrSort > 0 && uint32(zoneOrSort) != bot->GetZoneId()))
        {
            DropQuest(questId);
            ++dropped;
        }
    }

    if (dropped >= 8)
        return true;

    // Pass 3 (AC parity): still crammed — clear the log entirely and start fresh.
    for (uint16 slot = 0; slot < MAX_QUEST_LOG_SIZE; ++slot)
    {
        if (uint32 const questId = bot->GetQuestSlotQuestId(slot))
            DropQuest(questId);
    }

    return true;
}

void NewRpgBaseAction::DropQuest(uint32 questId)
{
    Player* bot = GetBot();
    if (!bot)
        return;

    uint16 const slot = bot->FindQuestSlot(questId);
    if (slot >= MAX_QUEST_LOG_SIZE)
        return;

    Quest const* quest = sObjectMgr->GetQuestTemplate(questId);

    // TC adaptation: AC feeds a synthetic CMSG_QUESTLOG_REMOVE_QUEST through the session
    // handler; here the handler's essential body is mirrored through public Player APIs (see
    // WorldSession::HandleQuestLogRemoveQuest). The no-abandon-once-begun refusal is checked
    // up front instead of the handler's post-removal early-return ordering.
    if (quest && quest->HasFlagEx(QUEST_FLAGS_EX_NO_ABANDON_ONCE_BEGUN))
        for (QuestObjective const& objective : quest->Objectives)
            if (bot->IsQuestObjectiveComplete(slot, quest, objective))
                return;

    if (!bot->TakeQuestSourceItem(questId, true))
        return; // can't un-equip some items — same rejection as the client path

    QuestStatus const oldStatus = bot->GetQuestStatus(questId);

    bot->RemoveActiveQuest(questId);

    if (quest)
    {
        if (quest->GetLimitTime())
            bot->RemoveTimedQuest(questId);

        if (quest->HasFlag(QUEST_FLAGS_FLAGS_PVP))
        {
            bot->pvpInfo.IsHostile = bot->pvpInfo.IsInHostileArea || bot->HasPvPForcingQuest();
            bot->UpdatePvPState();
        }
    }

    bot->SendForceSpawnTrackingUpdate(questId);
    bot->TakeQuestSourceItem(questId, true); // remove quest src item (mirrors the handler's second call)
    bot->AbandonQuest(questId);
    bot->DespawnPersonalSummonsForQuest(questId);

    sScriptMgr->OnQuestStatusChange(bot, questId);
    if (quest)
        sScriptMgr->OnQuestStatusChange(bot, quest, oldStatus, QUEST_STATUS_NONE);

    _botAI->GetRpgStatistics().questDropped++;
    TC_LOG_DEBUG("playerbots", "[New RPG] {} drop quest {}", bot->GetName(), questId);
}

bool NewRpgBaseAction::GetQuestPOIPosAndObjective(uint32 questId, std::vector<POIInfo>& poiInfo, bool toComplete)
{
    Player* bot = GetBot();
    if (!bot)
        return false;

    Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
    if (!quest)
        return false;

    QuestPOIData const* poiData = sObjectMgr->GetQuestPOIData(int32(questId));
    if (!poiData)
        return false;

    QuestStatus const status = bot->GetQuestStatus(questId);

    // Shared per-blob acceptance: weighted-average the blob polygon's points (modern
    // QuestPOIBlobPoint carries a real Z — no AC-style MAX_HEIGHT scan guessing), ground-snap
    // via map height around that Z, and keep it same-zone within 1500yd like AC.
    auto tryAppendBlob = [&](QuestPOIBlobData const& blob, uint32 objectiveId) -> void
    {
        if (blob.MapID != int32(bot->GetMapId()))
            return;

        if (blob.Points.empty())
            return;

        float dx = 0.0f, dy = 0.0f, dz = 0.0f;
        std::vector<float> const weights = GenerateRandomWeights(blob.Points.size());
        for (size_t i = 0; i < blob.Points.size(); ++i)
        {
            dx += float(blob.Points[i].X) * weights[i];
            dy += float(blob.Points[i].Y) * weights[i];
            dz += float(blob.Points[i].Z) * weights[i];
        }

        if (bot->GetDistance2d(dx, dy) >= 1500.0f)
            return;

        // Refine the interpolated Z to actual ground near it; a blob whose area has no
        // resolvable ground here is not a usable travel target.
        float const groundZ = bot->GetMap()->GetHeight(bot->GetPhaseShift(), dx, dy, dz + 5.0f);
        if (groundZ > INVALID_HEIGHT)
            dz = groundZ;

        if (dz <= INVALID_HEIGHT)
            return;

        if (bot->GetZoneId() != bot->GetMap()->GetZoneId(bot->GetPhaseShift(), dx, dy, dz))
            return;

        poiInfo.push_back({ Position(dx, dy, dz), objectiveId });
    };

    if (toComplete && status == QUEST_STATUS_COMPLETE)
    {
        // Turn-in travel target: the ObjectiveIndex == -1 blob marks the quest-ender's area
        // (same marker as WotLK).
        for (QuestPOIBlobData const& blob : poiData->Blobs)
        {
            if (blob.ObjectiveIndex != -1)
                continue;

            tryAppendBlob(blob, 0);
        }

        return !poiInfo.empty();
    }

    if (status != QUEST_STATUS_INCOMPLETE)
        return false;

    // Incomplete objectives, typed (Gate 10b E3/E5): pursue MONSTER/ITEM/GAMEOBJECT objectives
    // that are POI-locatable, keyed by QuestObjective::ID — never WotLK fixed-array index math.
    std::vector<QuestObjective const*> incompleteObjectives;
    uint16 const slot = bot->FindQuestSlot(questId);
    if (slot >= MAX_QUEST_LOG_SIZE)
        return false;

    for (QuestObjective const& objective : quest->Objectives)
    {
        switch (objective.Type)
        {
            case QUEST_OBJECTIVE_MONSTER:
            case QUEST_OBJECTIVE_ITEM:
            case QUEST_OBJECTIVE_GAMEOBJECT:
                break;
            default:
                continue; // other types (currency, reputation, areatrigger, ...) have no POI-travel meaning for V1
        }

        if (!bot->IsQuestObjectiveComplete(slot, quest, objective))
            incompleteObjectives.push_back(&objective);
    }

    if (incompleteObjectives.empty())
        return false;

    for (QuestPOIBlobData const& blob : poiData->Blobs)
    {
        if (blob.ObjectiveIndex == -1)
            continue; // turn-in blob — not an objective target

        // Primary key: blob.QuestObjectiveID matches QuestObjective::ID directly on modern
        // data. Fallback for older rows with QuestObjectiveID == 0: match the blob's
        // ObjectiveIndex against the objective's StorageIndex.
        QuestObjective const* matched = nullptr;
        for (QuestObjective const* objective : incompleteObjectives)
        {
            if (blob.QuestObjectiveID != 0
                ? uint32(blob.QuestObjectiveID) == objective->ID
                : blob.ObjectiveIndex == int32(objective->StorageIndex))
            {
                matched = objective;
                break;
            }
        }

        if (!matched)
            continue;

        tryAppendBlob(blob, matched->ID);
    }

    return !poiInfo.empty();
}

bool NewRpgBaseAction::RandomChangeStatus(std::vector<NewRpgStatus> const& candidateStatus)
{
    auto statusWeight = [](NewRpgStatus status) -> uint32
    {
        switch (status)
        {
            case RPG_WANDER_RANDOM:
                return Playerbots::GetRpgStatusProbWeightWanderRandom();
            case RPG_GO_GRIND:
                return Playerbots::GetRpgStatusProbWeightGoGrind();
            case RPG_GO_CAMP:
                return Playerbots::GetRpgStatusProbWeightGoCamp();
            case RPG_DO_QUEST:
                return Playerbots::GetRpgStatusProbWeightDoQuest();
            case RPG_WANDER_NPC:
                return Playerbots::GetRpgStatusProbWeightWanderNpc();
            case RPG_REST:
                return Playerbots::GetRpgStatusProbWeightRest();
            default:
                return 0;
        }
    };

    std::vector<NewRpgStatus> availableStatus;
    uint32 probSum = 0;
    for (NewRpgStatus status : candidateStatus)
    {
        if (!statusWeight(status))
            continue;

        if (CheckRpgStatusAvailable(status))
        {
            availableStatus.push_back(status);
            probSum += statusWeight(status);
        }
    }

    NewRpgInfo& info = _botAI->GetRpgInfo();
    Player* bot = GetBot();

    // Safety fallback when nothing is available or all weights are 0 (AC parity): sit the bot down
    // in RPG_REST. Reads better than aimless wandering and is AC-correct — WANDER_RANDOM itself
    // needs a live grind target nearby to even be available, so "nothing to do" -> sit.
    if (availableStatus.empty() || !probSum)
    {
        info.ChangeToRest();
        if (bot)
            bot->SetStandState(UNIT_STAND_STATE_SIT);
        return true;
    }

    uint32 const roll = urand(1, probSum);
    uint32 accumulate = 0;
    NewRpgStatus chosenStatus = RPG_STATUS_END;
    for (NewRpgStatus status : availableStatus)
    {
        accumulate += statusWeight(status);
        if (accumulate >= roll)
        {
            chosenStatus = status;
            break;
        }
    }

    switch (chosenStatus)
    {
        case RPG_WANDER_RANDOM:
            info.ChangeToWanderRandom();
            return true;
        case RPG_REST:
            info.ChangeToRest();
            if (bot)
                bot->SetStandState(UNIT_STAND_STATE_SIT);
            return true;
        case RPG_GO_GRIND:
        {
            Position dest;
            if (SelectRandomGrindPos(dest))
            {
                info.ChangeToGoGrind(dest);
                return true;
            }
            return false;
        }
        case RPG_GO_CAMP:
        {
            Position dest;
            if (SelectRandomHubPos(dest))
            {
                info.ChangeToGoCamp(dest);
                TC_LOG_DEBUG("playerbots", "[New RPG] {} travel to camp ({},{},{})",
                    bot ? bot->GetName() : "?", dest.GetPositionX(), dest.GetPositionY(), dest.GetPositionZ());
                return true;
            }
            return false;
        }
        case RPG_DO_QUEST:
        {
            uint32 questId = 0;
            Quest const* quest = nullptr;
            if (SelectRandomDoQuest(questId, quest))
            {
                info.ChangeToDoQuest(questId, quest);
                TC_LOG_DEBUG("playerbots", "[New RPG] {} start to do quest {}",
                    bot ? bot->GetName() : "?", questId);
                return true;
            }
            return false;
        }
        case RPG_WANDER_NPC:
            // Availability already confirmed a giver or a >= 3-NPC hub is in range; the WANDER_NPC
            // action picks and cycles its own targets (AC parity — ChangeToWanderNpc takes no arg).
            info.ChangeToWanderNpc();
            TC_LOG_DEBUG("playerbots", "[New RPG] {} start to wander npc hub", bot ? bot->GetName() : "?");
            return true;
        default:
            info.ChangeToIdle();
            return true;
    }
}

bool NewRpgBaseAction::CheckRpgStatusAvailable(NewRpgStatus status)
{
    Player* bot = GetBot();
    if (!bot)
        return false;

    switch (status)
    {
        case RPG_IDLE:
        case RPG_REST:
            // REST is always available (AC parity) — it's the "sit down, there's nothing pressing"
            // filler and the empty-availability fallback.
            return true;
        case RPG_WANDER_RANDOM:
            // AC gates wandering on a live grind target being around — pointless to meander in a
            // dead area (the RandomChangeStatus fallback sits the bot down as last resort).
            return FindNearbyAttackableUnit(bot, RPG_NEARBY_HOSTILE_RADIUS) != nullptr;
        case RPG_GO_GRIND:
        {
            Position dest;
            return SelectRandomGrindPos(dest);
        }
        case RPG_GO_CAMP:
        {
            Position dest;
            return SelectRandomHubPos(dest);
        }
        case RPG_DO_QUEST:
        {
            uint32 questId = 0;
            Quest const* quest = nullptr;
            return SelectRandomDoQuest(questId, quest);
        }
        case RPG_WANDER_NPC:
        {
            // AC's rule: available when there's an actionable giver to seek (the un-stranding
            // trigger) OR a genuine hub of >= 3 allowed-flag NPCs to mingle in.
            ObjectGuid nearestGiver;
            std::vector<ObjectGuid> hubNpcs;
            ScanWanderNpcTargets(nearestGiver, hubNpcs);
            return !nearestGiver.IsEmpty() || hubNpcs.size() >= HUB_MIN_NPCS;
        }
        default:
            return false;
    }
}

bool NewRpgBaseAction::SelectRandomGrindPos(Position& destOut)
{
    Player* bot = GetBot();
    if (!bot)
        return false;

    // AC: NewRpgBaseAction::SelectRandomGrindPos — 500yd "near" pool + 2500yd "far" pool over
    // the per-level destination cache, both ranges cut to a third below level 5, 50/50 chance
    // of preferring the near pool. TC source is Gate 10's GrindLocationCache (built from
    // sObjectMgr creature data like AC's TravelMgr cache).
    std::vector<GrindSpot> const& spots = sGrindLocationCache->GetSpotsForMap(bot->GetMapId());
    if (spots.empty())
        return false;

    float hiRange = 500.0f;
    float loRange = 2500.0f;
    if (bot->GetLevel() < 5)
    {
        hiRange /= 3;
        loRange /= 3;
    }

    std::vector<GrindSpot const*> loPool, hiPool;
    for (GrindSpot const& spot : spots)
    {
        if (!IsSpotForBotLevel(bot, spot))
            continue;

        float const dist = bot->GetExactDist(spot.Pos);
        if (dist > loRange)
            continue;

        if (dist < hiRange)
            hiPool.push_back(&spot);

        loPool.push_back(&spot);
    }

    std::vector<GrindSpot const*> const& pool = (urand(1, 100) <= 50 && !hiPool.empty()) ? hiPool : loPool;
    if (pool.empty())
        return false;

    // AC zone-filters every candidate up front; Map::GetZoneId per spot is a real terrain query,
    // so sample-and-check a handful instead of scanning the whole pool (same outcome: only
    // same-zone destinations get picked). AC's capital-city exception is dropped — with the zone
    // filter always on, a bot parked in a city simply won't find same-zone grind spots and
    // GO_GRIND reads unavailable there, which is fine for V1.
    for (int attempt = 0; attempt < 5; ++attempt)
    {
        GrindSpot const* spot = pool[urand(0, uint32(pool.size()) - 1)];
        if (bot->GetMap()->GetZoneId(bot->GetPhaseShift(), spot->Pos) != bot->GetZoneId())
            continue;

        destOut = spot->Pos;
        return true;
    }

    return false;
}

bool NewRpgBaseAction::SelectRandomDoQuest(uint32& questIdOut, Quest const*& questOut)
{
    Player* bot = GetBot();
    if (!bot)
        return false;

    std::unordered_set<uint32> const& lowPriority = _botAI->GetLowPriorityQuests();

    std::vector<uint32> availableQuests;
    for (uint16 slot = 0; slot < MAX_QUEST_LOG_SIZE; ++slot)
    {
        uint32 const questId = bot->GetQuestSlotQuestId(slot);
        if (!questId)
            continue;

        if (lowPriority.contains(questId))
            continue;

        // Skip provably-unactionable quests (no-ender COMPLETE, e.g. 55660) — classify once and
        // never re-evaluate them (handoff §4). In-memory only; the quest stays in the bot's log.
        if (IsQuestUnactionable(questId))
            continue;

        std::vector<POIInfo> poiInfo;
        if (GetQuestPOIPosAndObjective(questId, poiInfo, true))
            availableQuests.push_back(questId);
    }

    if (availableQuests.empty())
        return false;

    uint32 const questId = availableQuests[urand(0, uint32(availableQuests.size()) - 1)];
    Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
    if (!quest)
        return false;

    questIdOut = questId;
    questOut = quest;
    return true;
}
