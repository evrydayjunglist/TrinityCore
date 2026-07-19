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

#ifndef TRINITY_PLAYERBOT_COMBAT_TARGET_SELECT_H
#define TRINITY_PLAYERBOT_COMBAT_TARGET_SELECT_H

class BotPlayerbotAI;
class Player;
class Unit;

// Gate 12 shared combat target priority (AI_VALUE-backed):
//   1. valid "master target"
//   2. nearest valid unit from "attackers"
// AttackAnythingAction keeps its own RPG grind/quest search as priority 3 for masterless bots.
Unit* SelectCombatTarget(BotPlayerbotAI* botAI, Player* bot);
Unit* SelectNearestValidAttacker(BotPlayerbotAI* botAI, Player* bot);
bool HasValidCombatTarget(BotPlayerbotAI* botAI, Player* bot);

#endif
