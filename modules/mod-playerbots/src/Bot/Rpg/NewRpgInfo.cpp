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

#include "NewRpgInfo.h"
#include "Timer.h"
#include <sstream>

// AC reference: mod-playerbots-master/src/Ai/World/Rpg/NewRpgInfo.cpp — same ChangeTo*/
// GetStatus/ToString structure, TC subset of statuses.

void NewRpgInfo::ChangeToGoGrind(Position pos)
{
    startT = getMSTime();
    data = GoGrind{ pos };
}

void NewRpgInfo::ChangeToWanderRandom()
{
    startT = getMSTime();
    data = WanderRandom{};
}

void NewRpgInfo::ChangeToDoQuest(uint32 questId, Quest const* quest)
{
    startT = getMSTime();
    DoQuest doQuest;
    doQuest.questId = questId;
    doQuest.quest = quest;
    data = doQuest;
}

void NewRpgInfo::ChangeToIdle()
{
    startT = getMSTime();
    data = Idle{};
}

void NewRpgInfo::Reset()
{
    startT = getMSTime();
    data = Idle{};
    nearestMoveFarDis = std::numeric_limits<float>::max();
    stuckTs = 0;
    stuckAttempts = 0;
    hasMoveFarPos = false;
}

void NewRpgInfo::SetMoveFarTo(Position pos)
{
    nearestMoveFarDis = std::numeric_limits<float>::max();
    stuckTs = 0;
    stuckAttempts = 0;
    moveFarPos = pos;
    hasMoveFarPos = true;
}

NewRpgStatus NewRpgInfo::GetStatus() const
{
    return std::visit([](auto&& arg) -> NewRpgStatus
    {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, GoGrind>)
            return RPG_GO_GRIND;
        if constexpr (std::is_same_v<T, WanderRandom>)
            return RPG_WANDER_RANDOM;
        if constexpr (std::is_same_v<T, DoQuest>)
            return RPG_DO_QUEST;
        return RPG_IDLE;
    }, data);
}

bool NewRpgInfo::HasStatusPersisted(uint32 maxDurationMs) const
{
    return GetMSTimeDiffToNow(startT) > maxDurationMs;
}

std::string NewRpgInfo::ToString() const
{
    std::stringstream out;
    out << "Status: ";
    std::visit([&out, this](auto&& arg)
    {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, GoGrind>)
        {
            out << "GO_GRIND";
            out << " GrindPos: " << arg.pos.GetPositionX() << " " << arg.pos.GetPositionY()
                << " " << arg.pos.GetPositionZ();
        }
        else if constexpr (std::is_same_v<T, WanderRandom>)
            out << "WANDER_RANDOM";
        else if constexpr (std::is_same_v<T, DoQuest>)
        {
            out << "DO_QUEST";
            out << " questId: " << arg.questId;
            out << " objectiveId: " << arg.objectiveId;
            if (arg.hasPos)
            {
                out << " poiPos: " << arg.pos.GetPositionX() << " " << arg.pos.GetPositionY()
                    << " " << arg.pos.GetPositionZ();
            }
            out << " lastReachPOI: " << (arg.lastReachPOI ? GetMSTimeDiffToNow(arg.lastReachPOI) : 0);
        }
        else
            out << "IDLE";
    }, data);
    return out.str();
}
