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

#ifndef TRINITY_RANDOM_PLAYERBOT_FACTORY_H
#define TRINITY_RANDOM_PLAYERBOT_FACTORY_H

#include "Define.h"

// AC-shaped (RandomPlayerbotFactory), TC-native random-bot generation.
//
// Provisions reserved bot accounts (AC "<prefix><N>" model, Bnet-linked) and creates level-1 bot
// characters with a valid random race/class/gender/appearance/name, so RandomPlayerbotMgr's roster
// can be enumerated from the generated pool instead of a hand-list. Runs once at startup, before
// RandomPlayerbotMgr::BuildRoster(). Loadout/gear/talents and random initial level are a separate
// future project (out of scope). See playerbots-random-bot-generation-handoff.md.
class RandomPlayerbotFactory
{
public:
    // Entry point. No-op unless the random-bot feature is enabled (Playerbots.Enable +
    // Playerbots.MaxRandomBots > 0). Idempotent: only the shortfall of accounts/characters is
    // created on each run. If Playerbots.DeleteRandomBotAccounts is set, runs the teardown path
    // instead (deletes bot data and stops the server for a clean regenerate).
    // Provisions reserved bot accounts + characters up to MaxRandomBots (idempotent shortfall only).
    // Returns the number of NEW characters created this run (0 if none / feature off / teardown).
    // NB: character persistence is an ASYNC CharacterDatabase commit — callers reading the rows back
    // must drain the queue first (RandomPlayerbotMgr::Init does this when the return value is > 0).
    static uint32 GenerateRandomBots();
};

#endif
