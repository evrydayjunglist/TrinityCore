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

#include "PlayerbotMgr.h"
#include "BotPlayerbotAI.h"
#include "BotSessionMgr.h"
#include "CharacterCache.h"
#include "Chat.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "Log.h"
#include "Playerbots.h"
#include "PlayerbotsConfig.h"
#include "PlayerbotsMgr.h"
#include "SharedDefines.h"
#include "WorldSession.h"

PlayerbotMgr::PlayerbotMgr(Player* master) : _master(master)
{
}

bool PlayerbotMgr::AddBot(std::string const& name, ChatHandler* handler)
{
    if (!Playerbots::IsEnabled())
    {
        handler->SendSysMessage("Playerbots: module loaded, disabled (Playerbots.Enable = 0).");
        return true;
    }

    if (GetBotCount() >= Playerbots::GetMaxAddedBots())
    {
        handler->PSendSysMessage("Playerbots: you have added too many bots (max %u).", Playerbots::GetMaxAddedBots());
        return false;
    }

    return sBotSessionMgr->LoginMasterAlt(_master, name, handler);
}

bool PlayerbotMgr::RemoveBot(std::string const& name, ChatHandler* handler)
{
    return sBotSessionMgr->LogoutMasterAlt(_master, handler, &name);
}

void PlayerbotMgr::LogoutAllBots()
{
    sBotSessionMgr->LogoutMasterAlt(_master, nullptr, nullptr);
}

void PlayerbotMgr::ListBots(ChatHandler* handler) const
{
    if (_botGuids.empty())
    {
        handler->SendSysMessage("Playerbots: no active master-alt bots.");
        return;
    }

    handler->SendSysMessage("Playerbots: your active bots:");
    for (ObjectGuid const& guid : _botGuids)
    {
        if (Player* bot = ObjectAccessor::FindConnectedPlayer(guid))
            handler->PSendSysMessage("  - %s (%s)", bot->GetName().c_str(), guid.ToString().c_str());
        else
            handler->PSendSysMessage("  - %s", guid.ToString().c_str());
    }
}

void PlayerbotMgr::OnBotLogin(Player* bot)
{
    if (!bot)
        return;

    _botGuids.insert(bot->GetGUID());

    // AC: PlayerbotMgr::OnBotLoginInternal → SetMaster + ResetStrategies (Gate 8 prep).
    BotPlayerbotAI* botAI = GET_PLAYERBOT_AI(bot);
    if (!botAI)
        return;

    botAI->SetMaster(_master);
    botAI->ResetStrategies();

    if (Playerbots::GetLogLevel() >= 1)
    {
        TC_LOG_DEBUG("playerbots", "PlayerbotMgr::OnBotLogin bot={} master={}",
            bot->GetName(), _master ? _master->GetName() : "?");
    }
}

void PlayerbotMgr::OnBotLogout(ObjectGuid botGuid)
{
    _botGuids.erase(botGuid);

    if (Player* bot = ObjectAccessor::FindConnectedPlayer(botGuid))
        if (BotPlayerbotAI* botAI = GET_PLAYERBOT_AI(bot))
            botAI->SetMaster(nullptr);
}
