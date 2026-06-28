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

#include "ScriptMgr.h"
#include "Chat.h"
#include "ChatCommand.h"
#include "PlayerbotsConfig.h"
#include "RBAC.h"

using namespace Trinity::ChatCommands;

class playerbots_commandscript : public CommandScript
{
public:
    playerbots_commandscript() : CommandScript("playerbots_commandscript") { }

    std::span<ChatCommandBuilder const> GetCommands() const override
    {
        static ChatCommandTable playerbotsCommandTable =
        {
            { "status", HandlePlayerbotsStatusCommand, rbac::RBAC_PERM_COMMAND_HELP, Console::No },
        };

        static ChatCommandTable commandTable =
        {
            { "playerbots", playerbotsCommandTable },
        };
        return commandTable;
    }

    static bool HandlePlayerbotsStatusCommand(ChatHandler* handler)
    {
        if (Playerbots::IsEnabled())
            handler->SendSysMessage("Playerbots: module loaded, enabled (Playerbots.Enable = 1).");
        else
            handler->SendSysMessage("Playerbots: module loaded, disabled (Playerbots.Enable = 0).");

        return true;
    }
};

void AddSC_playerbots_commandscript()
{
    new playerbots_commandscript();
}
