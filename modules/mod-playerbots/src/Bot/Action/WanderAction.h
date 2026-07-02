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

#ifndef TRINITY_PLAYERBOT_WANDER_ACTION_H
#define TRINITY_PLAYERBOT_WANDER_ACTION_H

#include "Action.h"

class BotPlayerbotAI;

// Gate 10 — NewRpgStrategy "wander" sub-behavior (AC: RPG_WANDER_RANDOM). Lowest-relevance
// default action in the strategy: short random-offset idle movement so a random bot with no
// grind spot or quest giver nearby doesn't stand still.
class WanderAction : public Action
{
public:
    WanderAction(BotPlayerbotAI* botAI) : Action(botAI, "wander") { }

    bool Execute(Event event) override;
    bool IsUseful() override;
};

#endif
