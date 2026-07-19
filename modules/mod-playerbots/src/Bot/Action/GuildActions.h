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

#ifndef TRINITY_PLAYERBOT_GUILD_ACTIONS_H
#define TRINITY_PLAYERBOT_GUILD_ACTIONS_H

#include "Action.h"

class BotPlayerbotAI;

// AC: GuildAcceptAction — accept a pending guild invite from the bot's master's guild.
// Socketless bots call TC HandleGuildAcceptInvite (same path as CMSG_ACCEPT_GUILD_INVITE).
class GuildAcceptAction : public Action
{
public:
    explicit GuildAcceptAction(BotPlayerbotAI* botAI) : Action(botAI, "guild accept") { }

    bool Execute(Event event) override;
    bool IsUseful() override;
};

#endif
