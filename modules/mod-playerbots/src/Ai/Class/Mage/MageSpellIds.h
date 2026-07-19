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

#ifndef TRINITY_PLAYERBOT_MAGE_SPELL_IDS_H
#define TRINITY_PLAYERBOT_MAGE_SPELL_IDS_H

#include "DBCEnums.h"
#include "Define.h"
#include "Player.h"
#include "SharedDefines.h"

// Gate 15a Wave 1 — Midnight Frost spell ids from local SpellName/TraitDefinition/
// SpellClassOptions via EvryDb2Export build 12.0.7.67808 (handoff client 12.0.7.68453).
// No WotLK AC paste. Ice Barrier 414661 rejected (not TraitDefinition-granted).
namespace Playerbots::Mage::Frost
{
constexpr uint32 SPELL_FROSTBOLT   = 116;     // SpellName: Frostbolt (SLA + Spec baseline)
constexpr uint32 SPELL_ICE_LANCE   = 30455;   // SpellName: Ice Lance (TraitDefinition)
constexpr uint32 SPELL_ICE_BARRIER = 11426;   // SpellName: Ice Barrier (TraitDefinition)

inline bool IsFrostMage(Player const* player)
{
    return player
        && player->GetClass() == CLASS_MAGE
        && player->GetPrimarySpecialization() == ChrSpecialization::MageFrost;
}
}

#endif
