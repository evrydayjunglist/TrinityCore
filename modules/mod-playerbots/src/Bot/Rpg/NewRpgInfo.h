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

#ifndef TRINITY_PLAYERBOT_NEW_RPG_INFO_H
#define TRINITY_PLAYERBOT_NEW_RPG_INFO_H

#include "Define.h"
#include "Position.h"
#include <limits>
#include <string>
#include <variant>

class Quest;

// Gate 10b — per-bot RPG state machine. AC reference: mod-playerbots-master/src/Ai/World/Rpg/
// NewRpgInfo.h — same variant-over-payload-structs shape, subset of AC's statuses (GoCamp/
// WanderNpc/Rest/TravelFlight/OutdoorPvP stay out per the Gate 10b handoff scope; hub/rest/
// flight need a Midnight zone-level source first).
enum NewRpgStatus : uint8
{
    RPG_IDLE          = 0,
    RPG_GO_GRIND      = 1,
    RPG_WANDER_RANDOM = 2,
    RPG_DO_QUEST      = 3,
    RPG_STATUS_END
};

struct NewRpgInfo
{
    struct Idle
    {
    };

    // RPG_GO_GRIND — travel to a grind-location-cache spot, then hand over to WanderRandom.
    struct GoGrind
    {
        Position pos;
    };

    // RPG_WANDER_RANDOM — small random legs around the current position.
    struct WanderRandom
    {
    };

    // RPG_DO_QUEST — goal-directed quest doing: travel to the pursued objective's POI (or the
    // ObjectiveIndex == -1 turn-in blob once complete) and let the always-on attack behavior
    // do the killing. TC adaptation vs AC's payload: the pursued objective is keyed by
    // QuestObjective::ID (modern blobs carry QuestObjectiveID directly), not AC's WotLK
    // fixed-array objectiveIdx — confirmed mandatory on real data (handoff §3/§5, E3).
    struct DoQuest
    {
        Quest const* quest = nullptr;
        uint32 questId = 0;
        uint32 objectiveId = 0;      // QuestObjective::ID being pursued; 0 while traveling to the turn-in blob
        Position pos;
        bool hasPos = false;         // AC compares pos != WorldPosition(); an explicit flag avoids float compares
        bool travelToTurnIn = false; // AC's objectiveIdx == -1 marker: turn-in blob resolved, heading there
        uint32 lastReachPOI = 0;     // ms timestamp of arrival at the POI; 0 = not arrived yet
    };

    NewRpgInfo() : data(Idle{}) { }

    uint32 startT = 0;  // ms timestamp of the current status' start (AC: NewRpgInfo::startT)

    // MOVE_FAR stuck tracking (AC: nearestMoveFarDis/stuckTs/stuckAttempts/moveFarPos)
    float nearestMoveFarDis = std::numeric_limits<float>::max();
    uint32 stuckTs = 0;
    uint32 stuckAttempts = 0;
    Position moveFarPos;
    bool hasMoveFarPos = false;

    using RpgData = std::variant<Idle, GoGrind, WanderRandom, DoQuest>;
    RpgData data;

    NewRpgStatus GetStatus() const;
    bool HasStatusPersisted(uint32 maxDurationMs) const;

    void ChangeToGoGrind(Position pos);
    void ChangeToWanderRandom();
    void ChangeToDoQuest(uint32 questId, Quest const* quest);
    void ChangeToIdle();
    void Reset();
    void SetMoveFarTo(Position pos);

    std::string ToString() const;
};

// Per-bot lifecycle counters (AC: NewRpgInfo.h NewRpgStatistic) — surfaced through the
// .playerbot command output instead of AC's whisper-based TellRpgStatusAction.
struct NewRpgStatistic
{
    uint32 questAccepted = 0;
    uint32 questCompleted = 0;
    uint32 questAbandoned = 0;
    uint32 questRewarded = 0;
    uint32 questDropped = 0;
    uint32 itemsLooted = 0;   // quest loot picked up from corpses/objects (LootAction)
    uint32 objectsUsed = 0;   // quest-objective gameobjects used (UseQuestObjectAction)
};

#endif
