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

#ifndef TRINITY_PLAYERBOT_BOT_TALENT_MGR_H
#define TRINITY_PLAYERBOT_BOT_TALENT_MGR_H

class Player;

// Gate 13 — AC-shaped Mgr/Talent (role of Talentspec), TC-native TraitMgr starter loadouts.
// Detect/ensure ChrSpecialization, then apply retail starter combat TraitConfig from DB2.
namespace BotTalentMgr
{
// Login (and optional level-up) entry: config-gated, idempotent, no QueuePacket.
// forceRefresh=true re-runs starter fill even when StarterBuild is already set (level-up).
void EnsureSpecAndStarterTraits(Player* bot, bool forceRefresh = false);
}

#endif
