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

#ifndef TRINITY_BOT_SESSION_MGR_H
#define TRINITY_BOT_SESSION_MGR_H

// TC socketless session tracker for GM `.playerbot login` / `logout` (Gates 3–4)
// and master-alt `.playerbot bot` (Gate 7).

#include "ObjectGuid.h"
#include <functional>
#include <string>
#include <unordered_map>

class ChatHandler;
class Player;
class WorldSession;

class BotSessionMgr
{
public:
    static BotSessionMgr* instance();

    size_t GetActiveBotCount() const { return _sessionsByCharacterGuid.size(); }

    bool LoginCharacter(ChatHandler* handler, std::string const& nameOrGuid);
    bool LoginReservedCharacter(ObjectGuid characterGuid, std::string const& displayName);
    bool LoginMasterAlt(Player* master, std::string const& name, ChatHandler* handler);
    bool LogoutCharacter(ChatHandler* handler, std::string const* nameOrNull);
    bool LogoutReservedCharacter(ObjectGuid characterGuid);
    bool LogoutMasterAlt(Player* master, ChatHandler* handler, std::string const* nameOrNull);

    void RegisterSession(WorldSession* session, ObjectGuid characterGuid);
    void UnregisterSession(WorldSession* session);

    WorldSession* GetSessionByAccountId(uint32 accountId) const;
    WorldSession* GetSessionByCharacterName(std::string const& name) const;
    ObjectGuid GetMasterGuidForBot(ObjectGuid botCharacterGuid) const;

    void ForEachActiveBot(std::function<void(WorldSession* session, ObjectGuid characterGuid)> const& callback) const;

private:
    enum class LoginPolicy
    {
        ReservedAccount,
        MasterAlt
    };

    BotSessionMgr() = default;

    static ObjectGuid ResolveCharacterGuid(std::string const& nameOrGuid);
    bool CanStartBotLogin(ChatHandler* handler, ObjectGuid characterGuid, uint32 accountId, LoginPolicy policy, Player* master) const;
    bool StartBotLogin(ChatHandler* handler, ObjectGuid characterGuid, uint32 accountId, std::string const& displayName,
        LoginPolicy policy, ObjectGuid masterGuid);
    bool LogoutBotSession(WorldSession* session, ChatHandler* handler, bool sendMessage);

    std::unordered_map<uint32, WorldSession*> _sessionsByAccountId;
    std::unordered_map<ObjectGuid, WorldSession*> _sessionsByCharacterGuid;
    std::unordered_map<ObjectGuid, ObjectGuid> _masterByBotCharacterGuid;
};

#define sBotSessionMgr BotSessionMgr::instance()

#endif
