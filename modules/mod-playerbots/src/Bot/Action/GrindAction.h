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

#ifndef TRINITY_PLAYERBOT_GRIND_ACTION_H
#define TRINITY_PLAYERBOT_GRIND_ACTION_H

#include "Action.h"

class BotPlayerbotAI;

// Gate 10 — NewRpgStrategy "grind" sub-behavior (AC: RPG_GO_GRIND / SelectRandomGrindPos).
// Layer 1 (GrindLocationCache) picks a nearby cached grind spot to walk toward; Layer 2
// (live grid search, reusing AttackValidity.h) picks the actual hostile to attack once close.
class GrindAction : public Action
{
public:
    GrindAction(BotPlayerbotAI* botAI) : Action(botAI, "grind") { }

    bool Execute(Event event) override;
    bool IsUseful() override;
};

#endif
