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

#ifndef TRINITY_PLAYERBOT_ROGUE_SPELL_IDS_H
#define TRINITY_PLAYERBOT_ROGUE_SPELL_IDS_H

#include "DBCEnums.h"
#include "Define.h"
#include "Player.h"
#include "SharedDefines.h"

// Gate 14 pilot — Midnight Assassination spell ids from local SpellName.db2 via EvryDb2Export
// build 12.0.7.67808 (DataDir WOWSTATIC; handoff client 12.0.7.68453). No WotLK AC paste.
namespace Playerbots::Rogue::Assassination
{
constexpr uint32 SPELL_GARROTE      = 703;     // SpellName: Garrote
constexpr uint32 SPELL_MUTILATE     = 1329;    // SpellName: Mutilate
constexpr uint32 SPELL_ENVENOM      = 32645;   // SpellName: Envenom
constexpr uint32 SPELL_CRIMSON_VIAL = 185311;  // SpellName: Crimson Vial

constexpr uint8 ENVENOM_MIN_COMBO_POINTS = 5;

inline bool IsAssassinationRogue(Player const* player)
{
    return player
        && player->GetClass() == CLASS_ROGUE
        && player->GetPrimarySpecialization() == ChrSpecialization::RogueAssassination;
}
}

#endif
