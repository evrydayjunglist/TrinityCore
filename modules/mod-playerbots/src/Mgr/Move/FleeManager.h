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

#ifndef TRINITY_PLAYERBOT_FLEE_MANAGER_H
#define TRINITY_PLAYERBOT_FLEE_MANAGER_H

#include <vector>

class Player;
class Unit;

// Gate 12 — AC-shaped FleeManager (Mgr/Move), TC-native: Position/Map coords only, no TravelMgr.
// Destinations are candidates only; callers MUST commit via SafeMovement::TryMoveToValidatedPoint.
class FleeManager
{
public:
    FleeManager(Player* bot, float maxAllowedDistance);

    bool CalculateDestination(float& rx, float& ry, float& rz);

private:
    struct FleePoint
    {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
        float sumDistance = 0.0f;
        float minDistance = 0.0f;
    };

    void CalculateDistanceToCreatures(FleePoint& point, std::vector<Unit*> const& hostiles) const;
    void CalculatePossibleDestinations(std::vector<FleePoint>& points, std::vector<Unit*> const& hostiles) const;
    FleePoint const* SelectOptimalDestination(std::vector<FleePoint> const& points) const;

    Player* _bot = nullptr;
    float _maxAllowedDistance = 0.0f;
    float _startX = 0.0f;
    float _startY = 0.0f;
    float _startZ = 0.0f;
};

#endif
