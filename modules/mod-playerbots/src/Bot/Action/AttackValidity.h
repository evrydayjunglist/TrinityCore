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

#include <cstdint>
#include <unordered_set>

class Player;
class Unit;

// Shared hostility/validity check for bot attack targets (not dead, not friendly,
// core IsValidAttackTarget, LOS). Extracted out of AttackAction.cpp's anonymous namespace
// (Gate 8) so Gate 10's GrindAction can reuse it without duplicating combat-validity logic —
// see playerbots-gate-10-world-rpg-slice-handoff.md § Phase C / "no duplicate combat logic".
bool IsValidAttackTarget(Player* bot, Unit* target);

// Gate 10 Layer 2 live target search ("what to attack right now") — nearest attackable hostile
// within radius, validated through IsValidAttackTarget. Moved here from GrindAction's anonymous
// namespace in Gate 10b so the always-on "attack anything" action (AC's grind-strategy kill role)
// and the RPG status-availability check can share it.
Unit* FindNearbyAttackableUnit(Player* bot, float radius);

// RPG combat/objective completion V1 (playerbots-rpg-combat-objective-completion-handoff.md).
// Creature entries the bot still needs to *kill* for an incomplete QUEST_OBJECTIVE_MONSTER
// objective in its own quest log — data-first, no hardcoded ids. Mirrors
// TalkToQuestNpcAction::CollectQuestTalkToEntries exactly, MONSTER instead of TALKTO.
void CollectQuestKillEntries(Player* bot, std::unordered_set<uint32_t>& entries);

// Nearest living creature whose entry is a current quest kill target (CollectQuestKillEntries)
// AND passes IsValidAttackTarget — which, unlike FindNearbyAttackableUnit's hostile-only core
// search, accepts *neutral* creatures. This is the narrow widening that lets bots kill neutral
// quest mobs (e.g. mottled boars) without griefing generic neutral wildlife: inclusion is gated
// entirely on "this entry is a live quest kill objective".
Unit* FindNearbyQuestKillTarget(Player* bot, float radius);

#endif
