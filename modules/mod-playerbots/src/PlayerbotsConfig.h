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

#ifndef TRINITY_PLAYERBOTS_CONFIG_H
#define TRINITY_PLAYERBOTS_CONFIG_H

#include "Config.h"
#include "StringConvert.h"
#include "Util.h"
#include <algorithm>
#include <string>
#include <string_view>
#include <vector>

namespace Playerbots
{
inline bool IsEnabled()
{
    return sConfigMgr->GetBoolDefault("Playerbots.Enable", false);
}

inline uint32 GetReservedAccountMinId()
{
    return sConfigMgr->GetIntDefault("Playerbots.ReservedAccount.MinId", 0);
}

inline uint32 GetReservedAccountMaxId()
{
    return sConfigMgr->GetIntDefault("Playerbots.ReservedAccount.MaxId", 0);
}

inline uint32 GetLogLevel()
{
    return sConfigMgr->GetIntDefault("Playerbots.LogLevel", 0);
}

inline uint32 GetReactDelay()
{
    return std::max<uint32>(sConfigMgr->GetIntDefault("Playerbots.ReactDelay", 100), 1u);
}

inline uint32 GetGlobalCooldown()
{
    return std::max<uint32>(sConfigMgr->GetIntDefault("Playerbots.GlobalCooldown", 500), 1u);
}

inline float GetFollowDistance()
{
    return std::max<float>(sConfigMgr->GetFloatDefault("Playerbots.FollowDistance", 3.0f), 0.5f);
}

inline uint32 GetMaxActiveBots()
{
    return std::max<uint32>(sConfigMgr->GetIntDefault("Playerbots.MaxActiveBots", 1), 1u);
}

inline bool AllowAccountBots()
{
    return sConfigMgr->GetBoolDefault("Playerbots.AllowAccountBots", true);
}

inline uint32 GetMaxAddedBots()
{
    return std::max<uint32>(sConfigMgr->GetIntDefault("Playerbots.MaxAddedBots", 1), 1u);
}

inline std::vector<uint32> GetReservedAccountIds()
{
    std::vector<uint32> ids;
    std::string const idsConfig = sConfigMgr->GetStringDefault("Playerbots.ReservedAccount.Ids", "");
    if (idsConfig.empty())
        return ids;

    for (auto token : Trinity::Tokenize(idsConfig, ',', false))
    {
        if (Optional<uint32> id = Trinity::StringTo<uint32>(token))
            ids.push_back(*id);
    }
    return ids;
}

inline bool IsReservedAccountPolicyConfigured()
{
    if (!GetReservedAccountIds().empty())
        return true;

    uint32 const minId = GetReservedAccountMinId();
    uint32 const maxId = GetReservedAccountMaxId();
    return minId != 0 && maxId != 0 && minId <= maxId;
}

inline bool IsReservedAccount(uint32 accountId)
{
    if (accountId == 0)
        return false;

    std::vector<uint32> const explicitIds = GetReservedAccountIds();
    if (!explicitIds.empty())
        return std::ranges::find(explicitIds, accountId) != explicitIds.end();

    uint32 const minId = GetReservedAccountMinId();
    uint32 const maxId = GetReservedAccountMaxId();
    if (minId != 0 && maxId != 0 && minId <= maxId)
        return accountId >= minId && accountId <= maxId;

    return false;
}

inline std::string GetReservedAccountPolicySummary()
{
    std::vector<uint32> const explicitIds = GetReservedAccountIds();
    if (!explicitIds.empty())
    {
        std::string result = "explicit Ids:";
        for (uint32 id : explicitIds)
            result += " " + std::to_string(id);
        return result;
    }

    uint32 const minId = GetReservedAccountMinId();
    uint32 const maxId = GetReservedAccountMaxId();
    if (minId != 0 && maxId != 0 && minId <= maxId)
        return "range MinId-MaxId: " + std::to_string(minId) + "-" + std::to_string(maxId);

    return "not configured";
}

inline bool GetEnableDatabases()
{
    return sConfigMgr->GetBoolDefault("Playerbots.Updates.EnableDatabases", false);
}

inline std::string GetPlayerbotsDatabaseInfo()
{
    return sConfigMgr->GetStringDefault("PlayerbotsDatabaseInfo", "");
}

inline uint32 GetMinRandomBots()
{
    return sConfigMgr->GetIntDefault("Playerbots.MinRandomBots", 0);
}

inline uint32 GetMaxRandomBots()
{
    return sConfigMgr->GetIntDefault("Playerbots.MaxRandomBots", 0);
}

inline bool GetRandomBotAutologin()
{
    return sConfigMgr->GetBoolDefault("Playerbots.RandomBotAutologin", false);
}

inline bool GetDisabledWithoutRealPlayer()
{
    return sConfigMgr->GetBoolDefault("Playerbots.DisabledWithoutRealPlayer", true);
}

inline bool IsRandomBotFeatureEnabled()
{
    return IsEnabled() && GetMaxRandomBots() > 0;
}

inline std::vector<uint32> ParseCommaUintList(std::string const& value)
{
    std::vector<uint32> ids;
    if (value.empty())
        return ids;

    for (auto token : Trinity::Tokenize(value, ',', false))
    {
        if (Optional<uint32> id = Trinity::StringTo<uint32>(token))
            ids.push_back(*id);
    }
    return ids;
}

inline std::vector<std::string> ParseCommaStringList(std::string const& value)
{
    std::vector<std::string> items;
    if (value.empty())
        return items;

    for (auto token : Trinity::Tokenize(value, ',', false))
    {
        std::string trimmed = std::string(token);
        if (!trimmed.empty())
            items.push_back(std::move(trimmed));
    }
    return items;
}

inline std::vector<uint32> GetRandomBotAccounts()
{
    return ParseCommaUintList(sConfigMgr->GetStringDefault("Playerbots.RandomBotAccounts", ""));
}

inline std::vector<std::string> GetRandomBotCharacterNames()
{
    return ParseCommaStringList(sConfigMgr->GetStringDefault("Playerbots.RandomBotCharacterNames", ""));
}
}

#endif
