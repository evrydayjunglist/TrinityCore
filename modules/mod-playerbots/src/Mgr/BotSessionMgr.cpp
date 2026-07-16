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

#include "BotSessionMgr.h"
#include "AccountMgr.h"
#include "CharacterCache.h"
#include "Chat.h"
#include "ObjectAccessor.h"
#include "ObjectGuid.h"
#include "Player.h"
#include "PlayerbotsConfig.h"
#include "RealmList.h"
#include "Log.h"
#include "World.h"
#include "WorldSession.h"

BotSessionMgr* BotSessionMgr::instance()
{
    static BotSessionMgr instance;
    return &instance;
}

namespace
{
void SendBotMessage(ChatHandler* handler, std::string const& message)
{
    if (handler)
        handler->SendSysMessage(message);
    else if (Playerbots::GetLogLevel() > 0)
        TC_LOG_DEBUG("playerbots", "{}", message);
}
}

ObjectGuid BotSessionMgr::ResolveCharacterGuid(std::string const& nameOrGuid)
{
    if (nameOrGuid.find("Player-") != std::string::npos || nameOrGuid.find("0x") == 0)
        return ObjectGuid::FromString(nameOrGuid);
    return sCharacterCache->GetCharacterGuidByName(nameOrGuid);
}

WorldSession* BotSessionMgr::GetSessionByCharacterName(std::string const& name) const
{
    ObjectGuid guid = sCharacterCache->GetCharacterGuidByName(name);
    if (guid.IsEmpty())
        return nullptr;

    auto itr = _sessionsByCharacterGuid.find(guid);
    return itr != _sessionsByCharacterGuid.end() ? itr->second : nullptr;
}

ObjectGuid BotSessionMgr::GetMasterGuidForBot(ObjectGuid botCharacterGuid) const
{
    auto itr = _masterByBotCharacterGuid.find(botCharacterGuid);
    return itr != _masterByBotCharacterGuid.end() ? itr->second : ObjectGuid::Empty;
}

void BotSessionMgr::ForEachActiveBot(std::function<void(WorldSession* session, ObjectGuid characterGuid)> const& callback) const
{
    for (auto const& [characterGuid, session] : _sessionsByCharacterGuid)
        callback(session, characterGuid);
}

void BotSessionMgr::RegisterSession(WorldSession* session, ObjectGuid characterGuid)
{
    _sessionsByCharacterGuid[characterGuid] = session;
}

void BotSessionMgr::UnregisterSession(WorldSession* session)
{
    if (!session)
        return;

    ObjectGuid characterGuid;
    if (Player* player = session->GetPlayer())
        characterGuid = player->GetGUID();

    if (!characterGuid.IsEmpty())
    {
        _sessionsByCharacterGuid.erase(characterGuid);
        _masterByBotCharacterGuid.erase(characterGuid);
    }
}

bool BotSessionMgr::CanStartBotLogin(ChatHandler* handler, ObjectGuid characterGuid, uint32 accountId,
    LoginPolicy policy, Player* master) const
{
    if (characterGuid.IsEmpty())
    {
        SendBotMessage(handler, "Playerbots: character not found.");
        return false;
    }

    if (!accountId)
    {
        SendBotMessage(handler, "Playerbots: character has no account.");
        return false;
    }

    // Socketless bot login never hits Battle.net / WorldSocket ban gates. Refuse here so
    // RandomBotAutologin (and .playerbot login / master-alt) cannot recreate sessions on a
    // banned account after BanAccount evicted them via RemoveBotSession.
    {
        std::string accountName;
        if (AccountMgr::GetName(accountId, accountName) && AccountMgr::IsBannedAccount(accountName))
        {
            if (handler)
                handler->PSendSysMessage("Playerbots: account %u (%s) is banned.", accountId, accountName.c_str());
            else if (Playerbots::GetLogLevel() > 0)
                TC_LOG_DEBUG("playerbots", "Playerbots: refusing bot login for {} — account {} ({}) is banned.",
                    characterGuid.ToString(), accountId, accountName);
            return false;
        }
    }

    if (_sessionsByCharacterGuid.contains(characterGuid))
    {
        SendBotMessage(handler, "Playerbots: character already has an active bot session.");
        return false;
    }

    // Cover the async login window (and the post-LogoutPlayer eviction window) where the World
    // bot-session map still owns the GUID but BotSessionMgr / ObjectAccessor do not yet.
    if (sWorld->FindBotSession(characterGuid))
    {
        SendBotMessage(handler, "Playerbots: character already has a bot session pending or active.");
        return false;
    }

    if (Player* existing = ObjectAccessor::FindConnectedPlayer(characterGuid))
    {
        if (handler)
            handler->PSendSysMessage("Playerbots: character '%s' is already online.", existing->GetName().c_str());
        return false;
    }

    if (GetActiveBotCount() >= Playerbots::GetMaxActiveBots())
    {
        if (handler)
            handler->PSendSysMessage("Playerbots: active bot limit reached (%zu/%u). Log out a bot or raise Playerbots.MaxActiveBots.",
                GetActiveBotCount(), Playerbots::GetMaxActiveBots());
        return false;
    }

    if (policy == LoginPolicy::ReservedAccount)
    {
        if (!Playerbots::IsReservedAccount(accountId))
        {
            if (handler)
                handler->PSendSysMessage("Playerbots: account %u is not in the reserved bot account list.", accountId);
            return false;
        }

        if (sWorld->FindSession(accountId))
        {
            if (handler)
                handler->PSendSysMessage("Playerbots: account %u already has an active session.", accountId);
            return false;
        }

        // No account-level "already has a bot" gate here on purpose — a reserved account can
        // legitimately host several simultaneous random-bot characters (AC parity). The real
        // caps are the per-character check above (_sessionsByCharacterGuid.contains), the global
        // Playerbots.MaxActiveBots check above, and RandomPlayerbotMgr's own roster/MaxRandomBots
        // accounting. See playerbots-bot-session-account-cap-handoff.md.
        return true;
    }

    if (!master || !master->GetSession())
    {
        SendBotMessage(handler, "Playerbots: master player session not found.");
        return false;
    }

    if (!Playerbots::AllowAccountBots())
    {
        SendBotMessage(handler, "Playerbots: master-alt bots disabled (Playerbots.AllowAccountBots = 0).");
        return false;
    }

    uint32 const masterAccountId = master->GetSession()->GetAccountId();
    if (accountId != masterAccountId)
    {
        SendBotMessage(handler, "Playerbots: you can only add characters on your own account.");
        return false;
    }

    if (Playerbots::IsReservedAccount(accountId))
    {
        SendBotMessage(handler, "Playerbots: reserved-account characters use .playerbot login (GM), not .playerbot bot.");
        return false;
    }

    if (characterGuid == master->GetGUID())
    {
        SendBotMessage(handler, "Playerbots: you cannot add your current character as a bot.");
        return false;
    }

    // No account-level "already has a bot" gate here on purpose — Playerbots.MaxAddedBots
    // (checked by PlayerbotMgr::AddBot before this is even called) is the real per-master cap,
    // and it's meant to allow more than one alt when configured above 1. The per-character check
    // above (_sessionsByCharacterGuid.contains) still blocks logging the same character in twice.
    // See playerbots-bot-session-account-cap-handoff.md.
    return true;
}

bool BotSessionMgr::StartBotLogin(ChatHandler* handler, ObjectGuid characterGuid, uint32 accountId,
    std::string const& displayName, LoginPolicy policy, ObjectGuid masterGuid)
{
    std::string accountName;
    if (!AccountMgr::GetName(accountId, accountName))
    {
        if (handler)
            handler->PSendSysMessage("Playerbots: account %u not found.", accountId);
        return false;
    }

    AccountTypes const security = AccountTypes(AccountMgr::GetSecurity(accountId, int32(sRealmList->GetCurrentRealmId().Realm)));
    uint8 const expansion = uint8(sWorld->getIntConfig(CONFIG_EXPANSION));

    if (policy == LoginPolicy::MasterAlt && !masterGuid.IsEmpty())
        _masterByBotCharacterGuid[characterGuid] = masterGuid;

    WorldSession* session = WorldSession::CreateForBot(accountId, std::move(accountName), security, expansion);
    // Every bot session (reserved-account or master-alt) is tracked in World's GUID-keyed bot
    // map, never World::AddSession()'s account-keyed human map — see World::AddBotSession.
    if (!sWorld->AddBotSession(session, characterGuid))
    {
        // AddBotSession deletes `session` on duplicate-GUID refusal.
        if (policy == LoginPolicy::MasterAlt)
            _masterByBotCharacterGuid.erase(characterGuid);
        SendBotMessage(handler, "Playerbots: character already has a bot session pending or active.");
        return false;
    }

    session->LoginBotCharacter(characterGuid);

    if (handler)
        handler->PSendSysMessage("Playerbots: login requested for %s (%s) on account %u.",
            displayName.c_str(), characterGuid.ToString().c_str(), accountId);
    return true;
}

bool BotSessionMgr::LogoutBotSession(WorldSession* session, ChatHandler* handler, bool sendMessage)
{
    if (!session || !session->IsBotSession())
    {
        if (handler)
            handler->SendSysMessage("Playerbots: session is not a bot session.");
        return false;
    }

    uint32 const accountId = session->GetAccountId();
    session->LogoutPlayer(true);
    UnregisterSession(session);

    if (sendMessage && handler)
        handler->PSendSysMessage("Playerbots: bot on account %u logged out and saved.", accountId);

    return true;
}

bool BotSessionMgr::LoginCharacter(ChatHandler* handler, std::string const& nameOrGuid)
{
    if (!Playerbots::IsEnabled())
    {
        handler->SendSysMessage("Playerbots: module loaded, disabled (Playerbots.Enable = 0).");
        return true;
    }

    if (!Playerbots::IsReservedAccountPolicyConfigured())
    {
        handler->SendSysMessage("Playerbots: reserved account policy not configured (set Playerbots.ReservedAccount.Ids or MinId/MaxId).");
        return false;
    }

    ObjectGuid characterGuid = ResolveCharacterGuid(nameOrGuid);
    if (characterGuid.IsEmpty())
    {
        handler->PSendSysMessage("Playerbots: character '%s' not found.", nameOrGuid.c_str());
        return false;
    }

    uint32 const accountId = sCharacterCache->GetCharacterAccountIdByGuid(characterGuid);
    if (!CanStartBotLogin(handler, characterGuid, accountId, LoginPolicy::ReservedAccount, nullptr))
        return false;

    return StartBotLogin(handler, characterGuid, accountId, nameOrGuid, LoginPolicy::ReservedAccount, ObjectGuid::Empty);
}

bool BotSessionMgr::LoginReservedCharacter(ObjectGuid characterGuid, std::string const& displayName)
{
    if (!Playerbots::IsEnabled())
        return false;

    if (!Playerbots::IsReservedAccountPolicyConfigured())
        return false;

    if (characterGuid.IsEmpty())
        return false;

    uint32 const accountId = sCharacterCache->GetCharacterAccountIdByGuid(characterGuid);
    if (!CanStartBotLogin(nullptr, characterGuid, accountId, LoginPolicy::ReservedAccount, nullptr))
        return false;

    return StartBotLogin(nullptr, characterGuid, accountId, displayName, LoginPolicy::ReservedAccount, ObjectGuid::Empty);
}

bool BotSessionMgr::LogoutReservedCharacter(ObjectGuid characterGuid)
{
    if (!Playerbots::IsEnabled() || characterGuid.IsEmpty())
        return false;

    WorldSession* session = nullptr;
    auto itr = _sessionsByCharacterGuid.find(characterGuid);
    if (itr != _sessionsByCharacterGuid.end())
        session = itr->second;

    if (!session)
        return false;

    return LogoutBotSession(session, nullptr, false);
}

bool BotSessionMgr::LoginMasterAlt(Player* master, std::string const& name, ChatHandler* handler)
{
    if (!Playerbots::IsEnabled())
    {
        handler->SendSysMessage("Playerbots: module loaded, disabled (Playerbots.Enable = 0).");
        return true;
    }

    ObjectGuid characterGuid = ResolveCharacterGuid(name);
    if (characterGuid.IsEmpty())
    {
        handler->PSendSysMessage("Playerbots: character '%s' not found.", name.c_str());
        return false;
    }

    uint32 const accountId = sCharacterCache->GetCharacterAccountIdByGuid(characterGuid);
    if (!CanStartBotLogin(handler, characterGuid, accountId, LoginPolicy::MasterAlt, master))
        return false;

    return StartBotLogin(handler, characterGuid, accountId, name, LoginPolicy::MasterAlt, master->GetGUID());
}

bool BotSessionMgr::LogoutCharacter(ChatHandler* handler, std::string const* nameOrNull)
{
    if (!Playerbots::IsEnabled())
    {
        handler->SendSysMessage("Playerbots: module loaded, disabled (Playerbots.Enable = 0).");
        return true;
    }

    WorldSession* session = nullptr;

    if (nameOrNull && !nameOrNull->empty())
    {
        session = GetSessionByCharacterName(*nameOrNull);
        if (!session)
        {
            handler->PSendSysMessage("Playerbots: no active bot session for character '%s'.", nameOrNull->c_str());
            return false;
        }
    }
    else
    {
        if (_sessionsByCharacterGuid.size() == 1)
            session = _sessionsByCharacterGuid.begin()->second;
        else if (_sessionsByCharacterGuid.empty())
        {
            handler->SendSysMessage("Playerbots: no active bot sessions.");
            return false;
        }
        else
        {
            handler->SendSysMessage("Playerbots: multiple bot sessions active; specify character name.");
            return false;
        }
    }

    return LogoutBotSession(session, handler, true);
}

bool BotSessionMgr::LogoutMasterAlt(Player* master, ChatHandler* handler, std::string const* nameOrNull)
{
    if (!Playerbots::IsEnabled())
    {
        if (handler)
            handler->SendSysMessage("Playerbots: module loaded, disabled (Playerbots.Enable = 0).");
        return true;
    }

    if (!master)
        return false;

    ObjectGuid const masterGuid = master->GetGUID();

    if (nameOrNull && !nameOrNull->empty())
    {
        WorldSession* session = GetSessionByCharacterName(*nameOrNull);
        if (!session)
        {
            if (handler)
                handler->PSendSysMessage("Playerbots: no active bot session for character '%s'.", nameOrNull->c_str());
            return false;
        }

        if (GetMasterGuidForBot(sCharacterCache->GetCharacterGuidByName(*nameOrNull)) != masterGuid)
        {
            if (handler)
                handler->SendSysMessage("Playerbots: that character is not your master-alt bot.");
            return false;
        }

        return LogoutBotSession(session, handler, handler != nullptr);
    }

    bool loggedOutAny = false;
    std::vector<WorldSession*> toLogout;
    for (auto const& [botGuid, session] : _sessionsByCharacterGuid)
    {
        if (GetMasterGuidForBot(botGuid) == masterGuid)
            toLogout.push_back(session);
    }

    for (WorldSession* session : toLogout)
    {
        LogoutBotSession(session, handler, false);
        loggedOutAny = true;
    }

    if (!loggedOutAny && handler)
        handler->SendSysMessage("Playerbots: no active master-alt bots to log out.");

    return loggedOutAny;
}
