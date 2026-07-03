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
#include "Position.h"
#include <vector>

class BotPlayerbotAI;
class Quest;

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

private:
    // TC replacement for AC's synthetic CMSG_QUESTLOG_REMOVE_QUEST packet — mirrors the
    // essential body of WorldSession::HandleQuestLogRemoveQuest via public Player APIs.
    void DropQuest(uint32 questId);
};

#endif
