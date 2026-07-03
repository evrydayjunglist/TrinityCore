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

#include "AttackValidity.h"
#include "CellImpl.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "Player.h"
#include "Unit.h"

bool IsValidAttackTarget(Player* bot, Unit* target)
{
    if (!bot || !target || !target->IsInWorld())
        return false;

    if (target->isDead())
        return false;

    if (bot->IsFriendlyTo(target))
        return false;

    if (!bot->IsValidAttackTarget(target))
        return false;

    if (!bot->IsWithinLOSInMap(target))
        return false;

    return true;
}

Unit* FindNearbyAttackableUnit(Player* bot, float radius)
{
    if (!bot)
        return nullptr;

    Unit* victim = nullptr;
    Trinity::NearestAttackableUnitInObjectRangeCheck check(bot, bot, radius);
    Trinity::UnitLastSearcher<Trinity::NearestAttackableUnitInObjectRangeCheck> searcher(bot, victim, check);
    Cell::VisitAllObjects(bot, searcher, radius);

    if (victim && IsValidAttackTarget(bot, victim))
        return victim;

    return nullptr;
}
