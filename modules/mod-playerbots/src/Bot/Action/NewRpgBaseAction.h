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

#ifndef TRINITY_PLAYERBOT_NEW_RPG_BASE_ACTION_H
#define TRINITY_PLAYERBOT_NEW_RPG_BASE_ACTION_H

#include "Action.h"
#include "Bot/Rpg/NewRpgInfo.h"
#include "ObjectGuid.h"
#include "Position.h"
#include <unordered_set>
#include <vector>

class BotPlayerbotAI;
class Quest;
class WorldObject;

// One quest-POI travel candidate. AC reference: NewRpgBaseAction.h POIInfo {G3D::Vector2 pos,
// int32 objectiveIdx} — TC adaptations: full Position (modern QuestPOIBlobPoint carries a real
// Z, no MAX_HEIGHT scan guessing needed) and the pursued objective keyed by QuestObjective::ID
// (blob QuestObjectiveID / typed Quest::Objectives), not WotLK fixed-array index math
// (Gate 10b handoff §3/§5 E3 — confirmed mandatory on real Midnight data).
struct POIInfo
{
    Position pos;
    uint32 objectiveId = 0; // QuestObjective::ID; 0 = turn-in blob (ObjectiveIndex == -1)
};

// Gate 10b — shared helpers for the RPG status actions. AC reference: mod-playerbots-master/
// src/Ai/World/Rpg/Action/NewRpgBaseAction.{h,cpp} (their own comment: "we should make all
// actions composable instead of inheritable" — same composition-base approach here). Internals
// are TC-native: all movement goes through the SafeMovement contract (TryMoveToValidatedPoint /
// TryMoveTowardValidatedPoint), quest levels come from the core's own ContentTuning-aware
// Player::GetQuestLevel (E4 resolution), quest drops go through Player quest APIs instead of a
// synthetic CMSG_QUESTLOG_REMOVE_QUEST packet.
class NewRpgBaseAction : public Action
{
public:
    NewRpgBaseAction(BotPlayerbotAI* botAI, std::string name) : Action(botAI, std::move(name)) { }

protected:
    // --- movement (AC: MoveFarTo / MoveRandomNear) ---
    bool MoveFarTo(Position const& dest);
    bool MoveRandomNear(float moveStep = 20.0f);

    // --- quest worthiness (AC: IsQuestWorthDoing / IsQuestCapableDoing) ---
    bool IsQuestWorthDoing(Quest const* quest) const;
    bool IsQuestCapableDoing(Quest const* quest) const;

    // True iff this quest giver has at least one quest the bot could actually transact right now —
    // a COMPLETE quest it can be rewarded for, or a NONE quest it can take and that passes the
    // worth/capable filters. AC parity: NewRpgBaseAction::HasQuestToAcceptOrReward. Used by the
    // seek scan AND (convergence F1) QuestGiverAction's own candidate search, so a bot only ever
    // targets a giver it would really do business with — a menu holding just the bot's own
    // INCOMPLETE turn-ins doesn't count (that looser gate was the giver-anchor ping-pong bug).
    bool HasQuestToAcceptOrReward(WorldObject* questGiver) const;

    // A quest proven impossible for ANY player to finish: COMPLETE with no quest-ender in either
    // the creature or gameobject involved-relation data (e.g. auto-granted junk 55660). Safe to
    // ignore (never removed) — see handoff §4. Records the id in the per-session unactionable set.
    bool IsQuestUnactionable(uint32 questId) const;

    // --- lifelike hub mingling (AC: ChooseNpcOrGameObjectToInteract, RPG_WANDER_NPC) ---
    // Picks the next NPC for a WANDER_NPC cycle from a live grid scan (RpgWanderNpcRadius): an
    // actionable quest giver first (quest acquisition stays the priority), else a not-recently-
    // visited allowed-flag hub NPC (AC's allowedNpcFlags set). Returns false when there's neither a
    // giver nor a genuine hub (>= 3 allowed-flag NPCs) in range. No hardcoded ids, no cache (V1).
    bool SelectRandomNpcToInteract(ObjectGuid& targetOut, std::unordered_set<ObjectGuid> const& visited);

    // --- distant hub travel (AC: SelectRandomCampPos, RPG_GO_CAMP) ---
    // Picks the nearest same-zone HubLocationCache centroid that's worth traveling to (beyond ~50yd,
    // within the level-scaled range). Returns false when no such hub exists on this map/zone.
    bool SelectRandomHubPos(Position& destOut);

    // --- quest log hygiene (AC: OrganizeQuestLog) ---
    bool OrganizeQuestLog();

    // --- POI resolution (AC: GetQuestPOIPosAndObjectiveIdx) ---
    bool GetQuestPOIPosAndObjective(uint32 questId, std::vector<POIInfo>& poiInfo, bool toComplete = false);

    // --- status transitions (AC: RandomChangeStatus / CheckRpgStatusAvailable) ---
    bool RandomChangeStatus(std::vector<NewRpgStatus> const& candidateStatus);
    bool CheckRpgStatusAvailable(NewRpgStatus status);
    bool SelectRandomGrindPos(Position& destOut);
    bool SelectRandomDoQuest(uint32& questIdOut, Quest const*& questOut);

    // AC: NewRpgBaseAction.h pathFinderDis / stuckTime — same values.
    static constexpr float PATH_FINDER_DIS = 70.0f;
    static constexpr uint32 STUCK_TIME_MS = 90 * 1000;

    // Minimum allowed-flag NPCs in range for a spot to count as a genuine hub (AC's WANDER_NPC
    // ">= 3 possible targets" availability rule; matches HubLocationCache's HUB_CELL_MIN_SPAWNS).
    static constexpr size_t HUB_MIN_NPCS = 3;

private:
    // One shared grid scan (RpgWanderNpcRadius) feeding both WANDER_NPC availability and target
    // selection: the nearest actionable quest giver (empty if none) and the allowed-flag hub NPCs
    // in range, sorted nearest-first (spirit healers / hostiles / players excluded, AC's AcceptUnit).
    void ScanWanderNpcTargets(ObjectGuid& nearestGiverOut, std::vector<ObjectGuid>& hubNpcsOut);

    // TC replacement for AC's synthetic CMSG_QUESTLOG_REMOVE_QUEST packet — mirrors the
    // essential body of WorldSession::HandleQuestLogRemoveQuest via public Player APIs.
    void DropQuest(uint32 questId);
};

#endif
