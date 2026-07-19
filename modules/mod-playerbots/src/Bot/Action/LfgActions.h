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

#ifndef TRINITY_PLAYERBOT_LFG_ACTIONS_H
#define TRINITY_PLAYERBOT_LFG_ACTIONS_H

#include "Action.h"

class BotPlayerbotAI;

// AC: LfgRoleCheckAction — set roles during role check (V1 FollowMaster only).
// Midnight: construct DFSetRoles → HandleLfgSetRolesOpcode (not era CMSG_LFG_SET_ROLES).
// No QueuePacket / ResetStrategies / full AC GetRoles matrix.
class LfgRoleCheckAction : public Action
{
public:
    explicit LfgRoleCheckAction(BotPlayerbotAI* botAI) : Action(botAI, "lfg role check") { }

    bool Execute(Event event) override;
    bool IsUseful() override;
};

// AC: LfgAcceptAction — answer dungeon proposal (V1 FollowMaster only).
// Midnight: construct DFProposalResponse → HandleLfgProposalResultOpcode
// (not era CMSG_LFG_PROPOSAL_RESULT). Combat/dead → decline. No ResetStrategies / Refresh.
class LfgAcceptAction : public Action
{
public:
    explicit LfgAcceptAction(BotPlayerbotAI* botAI) : Action(botAI, "lfg accept") { }

    bool Execute(Event event) override;
    bool IsUseful() override;
};

#endif
