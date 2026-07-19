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

#include "FleeManager.h"
#include "AiObjectContext.h"
#include "Bot/Action/BotMapResidency.h"
#include "Bot/Engine/Value.h"
#include "BotPlayerbotAI.h"
#include "Map.h"
#include "ObjectAccessor.h"
#include "ObjectDefines.h"
#include "Player.h"
#include "Playerbots.h"
#include "PlayerbotsConfig.h"
#include "Unit.h"

#include <cmath>

namespace
{
constexpr float MIN_IMPROVE_DISTANCE = 3.0f;

bool IntersectsOri(float angle, std::vector<float> const& angles, float angleIncrement)
{
    for (float ori : angles)
        if (std::fabs(angle - ori) < angleIncrement)
            return true;
    return false;
}

std::vector<Unit*> CollectHostileUnits(Player* bot, BotPlayerbotAI* botAI)
{
    std::vector<Unit*> hostiles;
    if (!bot || !botAI)
        return hostiles;

    AiObjectContext* context = botAI->GetAiObjectContext();
    if (!context)
        return hostiles;

    Value<GuidVector>* attackersValue = context->GetValue<GuidVector>("attackers");
    if (!attackersValue)
        return hostiles;

    GuidVector const attackers = attackersValue->Get();
    hostiles.reserve(attackers.size());
    for (ObjectGuid const& guid : attackers)
    {
        if (Unit* unit = ObjectAccessor::GetUnit(*bot, guid))
            if (unit->IsAlive() && unit->IsInWorld())
                hostiles.push_back(unit);
    }
    return hostiles;
}
}

FleeManager::FleeManager(Player* bot, float maxAllowedDistance)
    : _bot(bot), _maxAllowedDistance(maxAllowedDistance)
{
    if (_bot)
    {
        _startX = _bot->GetPositionX();
        _startY = _bot->GetPositionY();
        _startZ = _bot->GetPositionZ();
    }
}

void FleeManager::CalculateDistanceToCreatures(FleePoint& point, std::vector<Unit*> const& hostiles) const
{
    point.minDistance = -1.0f;
    point.sumDistance = 0.0f;

    for (Unit* unit : hostiles)
    {
        if (!unit)
            continue;

        float const d = unit->GetExactDist2d(point.x, point.y);
        point.sumDistance += d;
        if (point.minDistance < 0.0f || point.minDistance > d)
            point.minDistance = d;
    }
}

void FleeManager::CalculatePossibleDestinations(std::vector<FleePoint>& points, std::vector<Unit*> const& hostiles) const
{
    if (!_bot || !_bot->IsInWorld())
        return;

    FleePoint start;
    start.x = _startX;
    start.y = _startY;
    start.z = _startZ;
    CalculateDistanceToCreatures(start, hostiles);

    std::vector<float> enemyOri;
    enemyOri.reserve(hostiles.size());
    for (Unit* unit : hostiles)
        enemyOri.push_back(_bot->GetAbsoluteAngle(unit));

    float const tooClose = std::max(Playerbots::GetFollowDistance(), 2.0f);
    float const distIncrement = std::max(tooClose, (_maxAllowedDistance - tooClose) / 8.0f);

    for (float dist = _maxAllowedDistance; dist >= tooClose; dist -= distIncrement)
    {
        float const angleIncrement = std::max(float(M_PI) / 16.0f,
            float(M_PI) / 4.0f / (1.0f + dist - tooClose));

        for (float add = 0.0f; add < float(M_PI) / 4.0f + angleIncrement; add += angleIncrement)
        {
            for (float angle = add; angle < add + 2.0f * float(M_PI) + angleIncrement; angle += float(M_PI) / 4.0f)
            {
                if (IntersectsOri(angle, enemyOri, angleIncrement))
                    continue;

                float x = _startX + std::cos(angle) * dist;
                float y = _startY + std::sin(angle) * dist;
                float z = _startZ + CONTACT_DISTANCE;

                // Freeze class 1: never GetHeight/IsInWater on a cold grid from map-thread AI.
                if (!IsBotMapPosQueryable(_bot, x, y))
                    continue;

                _bot->UpdateAllowedPositionZ(x, y, z);

                Map* map = _bot->GetMap();
                if (!map || map->IsInWater(_bot->GetPhaseShift(), x, y, z))
                    continue;

                FleePoint point;
                point.x = x;
                point.y = y;
                point.z = z;
                CalculateDistanceToCreatures(point, hostiles);

                if (point.minDistance < 0.0f)
                {
                    // No hostiles to score against — any dry walkable candidate is fine.
                    points.push_back(point);
                    continue;
                }

                if (point.minDistance - start.minDistance >= MIN_IMPROVE_DISTANCE)
                    points.push_back(point);
            }
        }
    }
}

FleeManager::FleePoint const* FleeManager::SelectOptimalDestination(std::vector<FleePoint> const& points) const
{
    FleePoint const* best = nullptr;
    for (FleePoint const& point : points)
        if (!best || point.sumDistance > best->sumDistance)
            best = &point;
    return best;
}

bool FleeManager::CalculateDestination(float& rx, float& ry, float& rz)
{
    if (!_bot)
        return false;

    BotPlayerbotAI* botAI = GET_PLAYERBOT_AI(_bot);
    std::vector<Unit*> const hostiles = CollectHostileUnits(_bot, botAI);

    std::vector<FleePoint> points;
    CalculatePossibleDestinations(points, hostiles);

    FleePoint const* point = SelectOptimalDestination(points);
    if (!point)
        return false;

    rx = point->x;
    ry = point->y;
    rz = point->z;
    return true;
}
