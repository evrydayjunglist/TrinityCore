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

#ifndef TRINITY_PLAYERBOT_GUILD_TRIGGERS_H
#define TRINITY_PLAYERBOT_GUILD_TRIGGERS_H

#include "Trigger.h"

class BotPlayerbotAI;

// Active while the bot has a pending guild invite into its master's guild.
// Poll fallback when payload observation is off or a signal is dropped.
class GuildInviteTrigger : public Trigger
{
public:
    explicit GuildInviteTrigger(BotPlayerbotAI* botAI) : Trigger(botAI, "guild invite") { }

    bool IsActive() override;
};

#endif
