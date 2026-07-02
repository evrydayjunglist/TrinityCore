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

#ifndef TRINITY_PLAYERBOT_ATTACK_VALIDITY_H
#define TRINITY_PLAYERBOT_ATTACK_VALIDITY_H

class Player;
class Unit;

// Shared hostility/validity check for bot attack targets (not dead, not friendly,
// core IsValidAttackTarget, LOS). Extracted out of AttackAction.cpp's anonymous namespace
// (Gate 8) so Gate 10's GrindAction can reuse it without duplicating combat-validity logic —
// see playerbots-gate-10-world-rpg-slice-handoff.md § Phase C / "no duplicate combat logic".
bool IsValidAttackTarget(Player* bot, Unit* target);

#endif
