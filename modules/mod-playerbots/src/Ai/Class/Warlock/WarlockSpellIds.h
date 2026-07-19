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

#ifndef TRINITY_PLAYERBOT_WARLOCK_SPELL_IDS_H
#define TRINITY_PLAYERBOT_WARLOCK_SPELL_IDS_H

#include "DBCEnums.h"
#include "Define.h"
#include "Player.h"
#include "SharedDefines.h"

// Gate 15a Wave 1 — Midnight Destruction spell ids from local SpellName/TraitDefinition/
// SpecializationSpells/SkillLineAbility via EvryDb2Export build 12.0.7.67808
// (handoff client 12.0.7.68453). Chaos Bolt 215279 rejected (instant damage companion).
namespace Playerbots::Warlock::Destruction
{
constexpr uint32 SPELL_IMMOLATE          = 348;     // SpellName: Immolate (SpecSpells 267) — cast
constexpr uint32 SPELL_IMMOLATE_DOT      = 157736;  // SpellName: Immolate — DoT aura (EffectAura 3)
                                                    // Cast 348 does NOT apply aura 348 on Midnight.
constexpr uint32 SPELL_INCINERATE        = 29722;   // SpellName: Incinerate (SpecSpells 267)
constexpr uint32 SPELL_CONFLAGRATE       = 17962;   // SpellName: Conflagrate (TraitDefinition)
constexpr uint32 SPELL_CHAOS_BOLT        = 116858;  // SpellName: Chaos Bolt (TraitDefinition)
constexpr uint32 SPELL_UNENDING_RESOLVE  = 104773;  // SpellName: Unending Resolve (SLA)

// SpellPower.db2 ManaCost=20, PowerType=7 (shards stored ×10; 20 = 2 shards).
constexpr int32 CHAOS_BOLT_MIN_SOUL_SHARDS = 20;

inline bool IsDestructionWarlock(Player const* player)
{
    return player
        && player->GetClass() == CLASS_WARLOCK
        && player->GetPrimarySpecialization() == ChrSpecialization::WarlockDestruction;
}
}

#endif
