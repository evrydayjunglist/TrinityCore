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

#ifndef TRINITY_PLAYERBOT_GROUP_ACTIONS_H
#define TRINITY_PLAYERBOT_GROUP_ACTIONS_H

#include "Action.h"

class BotPlayerbotAI;

// AC reference: mod-playerbots-master/src/Ai/Base/Actions/AcceptInvitationAction.cpp — accepts a
// pending party invite from the bot's master. AC drives HandleGroupAcceptOpcode off the invite
// SMSG; this fork's socketless bots have no inbound packet, so we call TC's own accept handler
// (HandlePartyInviteResponseOpcode) directly — the same server-side path the client's Accept
// button reaches.
class AcceptInvitationAction : public Action
{
public:
    explicit AcceptInvitationAction(BotPlayerbotAI* botAI) : Action(botAI, "accept invitation") { }

    bool Execute(Event event) override;
    bool IsUseful() override;
};

#endif
