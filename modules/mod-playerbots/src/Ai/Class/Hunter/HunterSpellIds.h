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

#ifndef TRINITY_PLAYERBOT_HUNTER_SPELL_IDS_H
#define TRINITY_PLAYERBOT_HUNTER_SPELL_IDS_H

#include "DBCEnums.h"
#include "Define.h"
#include "Player.h"
#include "SharedDefines.h"

// Gate 15a Wave 1 — Midnight Beast Mastery spell ids from local SpellName/TraitDefinition/
// SkillLineAbility via EvryDb2Export build 12.0.7.67808 (handoff client 12.0.7.68453).
// Kill Command 259489 rejected (Survival). Bestial Wrath 344572 rejected (pet/trigger).
namespace Playerbots::Hunter::BeastMastery
{
constexpr uint32 SPELL_KILL_COMMAND  = 34026;   // SpellName: Kill Command (TraitDefinition BM)
constexpr uint32 SPELL_BARBED_SHOT   = 217200;  // SpellName: Barbed Shot (TraitDefinition)
constexpr uint32 SPELL_COBRA_SHOT    = 193455;  // SpellName: Cobra Shot (TraitDefinition)
constexpr uint32 SPELL_EXHILARATION  = 109304;  // SpellName: Exhilaration (SkillLineAbility)

inline bool IsBeastMasteryHunter(Player const* player)
{
    return player
        && player->GetClass() == CLASS_HUNTER
        && player->GetPrimarySpecialization() == ChrSpecialization::HunterBeastMastery;
}
}

#endif
