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

#ifndef TRINITY_PLAYERBOT_DUEL_ACTIONS_H
#define TRINITY_PLAYERBOT_DUEL_ACTIONS_H

#include "Action.h"

class BotPlayerbotAI;

// AC: AcceptDuelAction — accept a master duel challenge (V1 FollowMaster only).
// Midnight: construct DuelResponse → HandleDuelResponseOpcode (not era CMSG_DUEL_ACCEPTED).
// No ResetStrategies / HP-cancel matrix / QueuePacket.
class AcceptDuelAction : public Action
{
public:
    explicit AcceptDuelAction(BotPlayerbotAI* botAI) : Action(botAI, "accept duel") { }

    bool Execute(Event event) override;
    bool IsUseful() override;
};

#endif
