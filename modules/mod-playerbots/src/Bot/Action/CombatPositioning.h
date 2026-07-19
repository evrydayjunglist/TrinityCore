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

#ifndef TRINITY_PLAYERBOT_COMBAT_POSITIONING_H
#define TRINITY_PLAYERBOT_COMBAT_POSITIONING_H

class Player;
class Unit;

// Gate 12 coarse melee/ranged role (not Gate 13 talent trees).
// PreferRanged config: -1 = ChrSpecialization Caster/Ranged heuristic, 0 = melee, 1 = ranged.
bool BotPrefersRangedCombat(Player const* bot);

// Apply class-agnostic combat movement for the current victim.
// Melee: MoveChase when not in melee and approach is walkable.
// Ranged: hold ~Playerbots.Combat.RangedDistance via SafeMovement; backpedal when too close.
// Returns true when movement was issued or the bot is already in an acceptable band.
bool ApplyCombatMovement(Player* bot, Unit* target);

#endif
