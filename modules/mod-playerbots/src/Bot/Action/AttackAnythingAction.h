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

#ifndef TRINITY_PLAYERBOT_ATTACK_ANYTHING_ACTION_H
#define TRINITY_PLAYERBOT_ATTACK_ANYTHING_ACTION_H

#include "Action.h"

class BotPlayerbotAI;

// Gate 10b — the always-on kill role for RPG bots, split out of Gate 10's GrindAction (which
// mixed "kill nearby" with "travel to grind spot"). AC reference: mod-playerbots-master's grind
// strategy runs its "attack anything" action alongside the newrpg strategy in every status —
// this is the fork equivalent, registered as a NewRpgStrategy default action so DoQuest/GoGrind/
// Wander all get their killing done by the same behavior. Also fights back when the bot is in
// combat without a victim (mob aggroed it, or its target died while others still attack).
class AttackAnythingAction : public Action
{
public:
    explicit AttackAnythingAction(BotPlayerbotAI* botAI) : Action(botAI, "attack anything") { }

    bool Execute(Event event) override;
    bool IsUseful() override;
};

#endif
