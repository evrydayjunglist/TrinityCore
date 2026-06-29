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

#ifndef TRINITY_PLAYERBOT_ATTACK_ACTION_H
#define TRINITY_PLAYERBOT_ATTACK_ACTION_H

#include "Action.h"

class BotPlayerbotAI;
class Player;
class Unit;

// AC reference: mod-playerbots-master/src/Ai/Base/Actions/AttackAction.h (minimal Gate 8)
bool HasAttackableMasterTarget(BotPlayerbotAI* botAI, Player* bot);

class AttackMyTargetAction : public Action
{
public:
    AttackMyTargetAction(BotPlayerbotAI* botAI) : Action(botAI, "attack my target") { }

    bool Execute(Event event) override;
    bool IsUseful() override;
};

#endif
