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

// AC reference: mod-playerbots-master/src/Script/PlayerbotCommandScript.cpp
// This fork: top-level `.playerbot` (singular). Master-alt control is `.playerbot bot`.

#include "ScriptMgr.h"
#include "BotSessionMgr.h"
#include "Chat.h"
#include "ChatCommand.h"
#include "Player.h"
#include "PlayerbotMgr.h"
#include "Playerbots.h"
#include "PlayerbotsConfig.h"
#include "PlayerbotsDatabaseMgr.h"
#include "RandomPlayerbotMgr.h"
#include "RBAC.h"
#include "WorldSession.h"

using namespace Trinity::ChatCommands;

class playerbots_commandscript : public CommandScript
{
public:
    playerbots_commandscript() : CommandScript("playerbots_commandscript") { }

    std::span<ChatCommandBuilder const> GetCommands() const override
    {
        static ChatCommandTable playerbotsBotCommandTable =
        {
            { "add",    HandlePlayerbotBotAddCommand,    rbac::RBAC_PERM_COMMAND_HELP, Console::No },
            { "remove", HandlePlayerbotBotRemoveCommand, rbac::RBAC_PERM_COMMAND_HELP, Console::No },
            { "list",   HandlePlayerbotBotListCommand,   rbac::RBAC_PERM_COMMAND_HELP, Console::No },
            { "logout", HandlePlayerbotBotLogoutCommand, rbac::RBAC_PERM_COMMAND_HELP, Console::No },
        };

        static ChatCommandTable playerbotsAccountCommandTable =
        {
            { "setKey",          HandlePlayerbotsNyiCommand, rbac::RBAC_PERM_COMMAND_HELP, Console::No },
            { "link",            HandlePlayerbotsNyiCommand, rbac::RBAC_PERM_COMMAND_HELP, Console::No },
            { "linkedAccounts",  HandlePlayerbotsNyiCommand, rbac::RBAC_PERM_COMMAND_HELP, Console::No },
            { "unlink",          HandlePlayerbotsNyiCommand, rbac::RBAC_PERM_COMMAND_HELP, Console::No },
        };

        static ChatCommandTable playerbotsRndbotCommandTable =
        {
            { "status", HandlePlayerbotRndbotStatusCommand, rbac::RBAC_PERM_COMMAND_GM, Console::No },
            { "start",  HandlePlayerbotRndbotStartCommand,  rbac::RBAC_PERM_COMMAND_GM, Console::No },
            { "stop",   HandlePlayerbotRndbotStopCommand,   rbac::RBAC_PERM_COMMAND_GM, Console::No },
        };

        static ChatCommandTable playerbotsCommandTable =
        {
            { "bot",    playerbotsBotCommandTable },
            { "rndbot", playerbotsRndbotCommandTable },
            { "account", playerbotsAccountCommandTable },
            // TC extensions (socketless GM bots on reserved accounts — Gates 3–4):
            { "login",  HandlePlayerbotsLoginCommand,     rbac::RBAC_PERM_COMMAND_GM,   Console::No },
            { "logout", HandlePlayerbotsLogoutCommand,    rbac::RBAC_PERM_COMMAND_GM,   Console::No },
            { "status", HandlePlayerbotsStatusCommand,    rbac::RBAC_PERM_COMMAND_HELP, Console::No },
        };

        static ChatCommandTable commandTable =
        {
            { "playerbot", playerbotsCommandTable },
        };
        return commandTable;
    }

    static bool HandlePlayerbotsStatusCommand(ChatHandler* handler)
    {
        if (Playerbots::IsEnabled())
        {
            uint32 const maxBots = Playerbots::GetMaxActiveBots();
            uint32 const activeCount = static_cast<uint32>(sBotSessionMgr->GetActiveBotCount());
            handler->PSendSysMessage("Playerbots: enabled. Active bots: %u/%u. Reserved accounts: %s.",
                activeCount, maxBots, Playerbots::GetReservedAccountPolicySummary().c_str());

            sBotSessionMgr->ForEachActiveBot([&](WorldSession* session, ObjectGuid /*characterGuid*/)
            {
                if (Player* bot = session->GetPlayer())
                    handler->PSendSysMessage("  - %s", bot->GetName().c_str());
            });

            handler->SendSysMessage("Playerbots: GM — .playerbot login/logout. Master-alt — .playerbot bot add/remove/list/logout.");
        }
        else
            handler->SendSysMessage("Playerbots: module loaded, disabled (Playerbots.Enable = 0).");

        return true;
    }

    static bool HandlePlayerbotsLoginCommand(ChatHandler* handler, std::string characterName)
    {
        return sBotSessionMgr->LoginCharacter(handler, characterName);
    }

    static bool HandlePlayerbotsLogoutCommand(ChatHandler* handler, Optional<std::string> characterName)
    {
        if (characterName)
            return sBotSessionMgr->LogoutCharacter(handler, &*characterName);
        return sBotSessionMgr->LogoutCharacter(handler, nullptr);
    }

    static bool HandlePlayerbotBotAddCommand(ChatHandler* handler, std::string characterName)
    {
        if (!Playerbots::IsEnabled())
        {
            handler->SendSysMessage("Playerbots: module loaded, disabled (Playerbots.Enable = 0).");
            return true;
        }

        Player* master = handler->GetSession() ? handler->GetSession()->GetPlayer() : nullptr;
        if (!master)
        {
            handler->SendSysMessage("Playerbots: .playerbot bot requires an in-game player session.");
            return false;
        }

        PlayerbotMgr* mgr = sPlayerbotsMgr->GetPlayerbotMgr(master);
        if (!mgr)
        {
            handler->SendSysMessage("Playerbots: PlayerbotMgr not found for your character.");
            return false;
        }

        return mgr->AddBot(characterName, handler);
    }

    static bool HandlePlayerbotBotRemoveCommand(ChatHandler* handler, std::string characterName)
    {
        if (!Playerbots::IsEnabled())
        {
            handler->SendSysMessage("Playerbots: module loaded, disabled (Playerbots.Enable = 0).");
            return true;
        }

        Player* master = handler->GetSession() ? handler->GetSession()->GetPlayer() : nullptr;
        if (!master)
        {
            handler->SendSysMessage("Playerbots: .playerbot bot requires an in-game player session.");
            return false;
        }

        PlayerbotMgr* mgr = sPlayerbotsMgr->GetPlayerbotMgr(master);
        if (!mgr)
        {
            handler->SendSysMessage("Playerbots: PlayerbotMgr not found for your character.");
            return false;
        }

        return mgr->RemoveBot(characterName, handler);
    }

    static bool HandlePlayerbotBotListCommand(ChatHandler* handler)
    {
        if (!Playerbots::IsEnabled())
        {
            handler->SendSysMessage("Playerbots: module loaded, disabled (Playerbots.Enable = 0).");
            return true;
        }

        Player* master = handler->GetSession() ? handler->GetSession()->GetPlayer() : nullptr;
        if (!master)
        {
            handler->SendSysMessage("Playerbots: .playerbot bot requires an in-game player session.");
            return false;
        }

        PlayerbotMgr* mgr = sPlayerbotsMgr->GetPlayerbotMgr(master);
        if (!mgr)
        {
            handler->SendSysMessage("Playerbots: PlayerbotMgr not found for your character.");
            return false;
        }

        mgr->ListBots(handler);
        return true;
    }

    static bool HandlePlayerbotBotLogoutCommand(ChatHandler* handler, Optional<std::string> characterName)
    {
        if (!Playerbots::IsEnabled())
        {
            handler->SendSysMessage("Playerbots: module loaded, disabled (Playerbots.Enable = 0).");
            return true;
        }

        Player* master = handler->GetSession() ? handler->GetSession()->GetPlayer() : nullptr;
        if (!master)
        {
            handler->SendSysMessage("Playerbots: .playerbot bot requires an in-game player session.");
            return false;
        }

        PlayerbotMgr* mgr = sPlayerbotsMgr->GetPlayerbotMgr(master);
        if (!mgr)
        {
            handler->SendSysMessage("Playerbots: PlayerbotMgr not found for your character.");
            return false;
        }

        if (characterName)
            return mgr->RemoveBot(*characterName, handler);

        mgr->LogoutAllBots();
        handler->SendSysMessage("Playerbots: logged out all your master-alt bots.");
        return true;
    }

    static bool HandlePlayerbotRndbotStatusCommand(ChatHandler* handler)
    {
        if (!Playerbots::IsEnabled())
        {
            handler->SendSysMessage("Playerbots: module loaded, disabled (Playerbots.Enable = 0).");
            return true;
        }

        handler->PSendSysMessage("Playerbots random bots: feature %s (MaxRandomBots=%u, MinRandomBots=%u).",
            Playerbots::IsRandomBotFeatureEnabled() ? "enabled" : "off",
            Playerbots::GetMaxRandomBots(), Playerbots::GetMinRandomBots());
        handler->PSendSysMessage("  Autologin: %s | DisabledWithoutRealPlayer: %s | Scheduler paused: %s",
            Playerbots::GetRandomBotAutologin() ? "yes" : "no",
            Playerbots::GetDisabledWithoutRealPlayer() ? "yes" : "no",
            sRandomPlayerbotMgr->IsSchedulerPaused() ? "yes" : "no");
        handler->PSendSysMessage("  Active random bots: %zu / target %zu",
            sRandomPlayerbotMgr->GetActiveRandomBotCount(), sRandomPlayerbotMgr->GetTargetRandomBotCount());
        handler->PSendSysMessage("  DB: %s", sPlayerbotsDatabaseMgr->GetStatusSummary().c_str());
        handler->PSendSysMessage("  Reserved accounts: %s", Playerbots::GetReservedAccountPolicySummary().c_str());
        return true;
    }

    static bool HandlePlayerbotRndbotStartCommand(ChatHandler* handler)
    {
        if (!Playerbots::IsEnabled())
        {
            handler->SendSysMessage("Playerbots: module loaded, disabled (Playerbots.Enable = 0).");
            return true;
        }

        if (!Playerbots::IsRandomBotFeatureEnabled())
        {
            handler->SendSysMessage("Playerbots: random bots off (set Playerbots.MaxRandomBots > 0).");
            return true;
        }

        sRandomPlayerbotMgr->SetSchedulerPaused(false);
        sRandomPlayerbotMgr->TriggerSchedulerPass();
        handler->SendSysMessage("Playerbots: random bot scheduler started (one pass requested).");
        return true;
    }

    static bool HandlePlayerbotRndbotStopCommand(ChatHandler* handler)
    {
        if (!Playerbots::IsEnabled())
        {
            handler->SendSysMessage("Playerbots: module loaded, disabled (Playerbots.Enable = 0).");
            return true;
        }

        sRandomPlayerbotMgr->SetSchedulerPaused(true);
        sRandomPlayerbotMgr->Shutdown();
        handler->SendSysMessage("Playerbots: random bots stopped and scheduler paused.");
        return true;
    }

    static bool HandlePlayerbotsNyiCommand(ChatHandler* handler)
    {
        if (!Playerbots::IsEnabled())
        {
            handler->SendSysMessage("Playerbots: module loaded, disabled (Playerbots.Enable = 0).");
            return true;
        }

        handler->SendSysMessage("Playerbots: not implemented.");
        return true;
    }
};

void AddPlayerbotsCommandscripts()
{
    new playerbots_commandscript();
}
