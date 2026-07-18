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

#ifndef TRINITY_PLAYERBOT_LEVEL_UP_ACTION_H
#define TRINITY_PLAYERBOT_LEVEL_UP_ACTION_H

#include "Action.h"

class BotPlayerbotAI;

// Minimal AC "levelup": signal wake-up + optional TellMaster "leveled to <Level>".
// No AutoMaintenanceOnLevelup (talents / spells / teleport / equip).
class LevelUpAction : public Action
{
public:
    explicit LevelUpAction(BotPlayerbotAI* botAI)
        : Action(botAI, "levelup") { }

    bool Execute(Event event) override;
    bool IsUseful() override;
};

#endif
