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

#include "RandomPlayerbotMgr.h"
#include "BotSessionMgr.h"
#include "CharacterCache.h"
#include "Containers.h"
#include "Log.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "PlayerbotsConfig.h"
#include "PlayerbotsDatabaseMgr.h"
#include "World.h"
#include "WorldSession.h"

namespace
{
uint32 constexpr RANDOM_BOT_UPDATE_INTERVAL_MS = 5000;
}

RandomPlayerbotMgr* RandomPlayerbotMgr::instance()
{
    static RandomPlayerbotMgr instance;
    return &instance;
}

bool RandomPlayerbotMgr::IsSchedulerEnabled() const
{
    return Playerbots::IsEnabled() && Playerbots::IsRandomBotFeatureEnabled();
}

size_t RandomPlayerbotMgr::GetTargetRandomBotCount() const
{
    if (!Playerbots::IsRandomBotFeatureEnabled())
        return 0;

    return Playerbots::GetMaxRandomBots();
}

void RandomPlayerbotMgr::Init()
{
    _roster.clear();
    _activeRandomBotGuids.clear();
    _updateTimer = 0;
    _saveTimer = 0;
    _schedulerPaused = !Playerbots::GetRandomBotAutologin();

    if (!IsSchedulerEnabled())
        return;

    if (Playerbots::GetEnableDatabases() && !sPlayerbotsDatabaseMgr->CheckVersion())
        TC_LOG_WARN("server.loading", "Playerbots: random bot DB enabled but version check failed — scheduler may be limited.");

    BuildRoster();
}

void RandomPlayerbotMgr::BuildRoster()
{
    _roster.clear();

    std::vector<uint32> const accounts = Playerbots::GetRandomBotAccounts();
    std::vector<std::string> const names = Playerbots::GetRandomBotCharacterNames();

    for (size_t i = 0; i < accounts.size(); ++i)
    {
        uint32 const accountId = accounts[i];
        if (!Playerbots::IsReservedAccount(accountId))
        {
            TC_LOG_ERROR("server.loading", "Playerbots: RandomBotAccounts entry {} is not a reserved account — skipped.", accountId);
            continue;
        }

        RandomBotRosterEntry entry;
        entry.AccountId = accountId;

        if (i < names.size() && !names[i].empty())
            entry.CharacterName = names[i];
        else
        {
            TC_LOG_WARN("server.loading", "Playerbots: no character name for random bot account {} — set Playerbots.RandomBotCharacterNames.", accountId);
            continue;
        }

        entry.CharacterGuid = sCharacterCache->GetCharacterGuidByName(entry.CharacterName);
        if (entry.CharacterGuid.IsEmpty())
        {
            TC_LOG_ERROR("server.loading", "Playerbots: random bot character '{}' not found.", entry.CharacterName);
            continue;
        }

        if (sCharacterCache->GetCharacterAccountIdByGuid(entry.CharacterGuid) != accountId)
        {
            TC_LOG_ERROR("server.loading", "Playerbots: character '{}' is not on account {}.", entry.CharacterName, accountId);
            continue;
        }

        _roster.push_back(std::move(entry));
    }
}

bool RandomPlayerbotMgr::HasRealPlayerOnline() const
{
    for (auto const& [accountId, session] : sWorld->GetAllSessions())
    {
        (void)accountId;
        if (!session || session->IsBotSession())
            continue;

        if (session->GetPlayer())
            return true;
    }
    return false;
}

void RandomPlayerbotMgr::Update(uint32 diff)
{
    if (!IsSchedulerEnabled() || _schedulerPaused)
        return;

    if (Playerbots::GetDisabledWithoutRealPlayer() && !HasRealPlayerOnline())
    {
        if (!_activeRandomBotGuids.empty())
            TryLogoutExcessRandomBots();
        return;
    }

    // Review follow-up C1: periodic DB flush, independent of the login/logout balancing timer
    // below (different cadence, must not be skipped by that timer's own early return).
    uint32 const saveIntervalSeconds = Playerbots::GetRandomBotSaveIntervalSeconds();
    if (saveIntervalSeconds > 0)
    {
        _saveTimer += diff;
        if (_saveTimer >= saveIntervalSeconds * 1000)
        {
            _saveTimer = 0;
            SaveActiveRandomBots();
        }
    }

    _updateTimer += diff;
    if (_updateTimer < RANDOM_BOT_UPDATE_INTERVAL_MS)
        return;

    _updateTimer = 0;
    TryLoginRandomBots();
    TryLogoutExcessRandomBots();
}

void RandomPlayerbotMgr::SaveActiveRandomBots()
{
    for (ObjectGuid const& guid : _activeRandomBotGuids)
    {
        Player* bot = ObjectAccessor::FindConnectedPlayer(guid);
        if (!bot || !bot->IsInWorld())
            continue;

        bot->SaveToDB(false);
    }
}

void RandomPlayerbotMgr::TriggerSchedulerPass()
{
    if (!IsSchedulerEnabled())
        return;

    _updateTimer = RANDOM_BOT_UPDATE_INTERVAL_MS;
    Update(0);
}

void RandomPlayerbotMgr::TryLoginRandomBots()
{
    size_t const target = GetTargetRandomBotCount();
    if (_activeRandomBotGuids.size() >= target)
        return;

    // AC picks the next bot to add at random from the available pool (RandomPlayerbotFactory /
    // AddRandomBots), rather than a fixed scan order. Shuffle the not-yet-active candidates each
    // pass so a roster larger than MaxRandomBots doesn't always favor the same declared-order
    // prefix — matters once the roster is a real pool rather than this fork's small dev fixture.
    std::vector<RandomBotRosterEntry const*> candidates;
    candidates.reserve(_roster.size());
    for (RandomBotRosterEntry const& entry : _roster)
        if (!_activeRandomBotGuids.contains(entry.CharacterGuid))
            candidates.push_back(&entry);

    Trinity::Containers::RandomShuffle(candidates);

    for (RandomBotRosterEntry const* entry : candidates)
    {
        if (_activeRandomBotGuids.size() >= target)
            break;

        // No account-level skip here on purpose — a reserved account can host several roster
        // entries (e.g. Three + Threethree both on account 3); the per-character check above
        // is the correct guard. See playerbots-bot-session-account-cap-handoff.md.
        LoginRosterEntry(*entry);
    }
}

void RandomPlayerbotMgr::TryLogoutExcessRandomBots()
{
    size_t target = GetTargetRandomBotCount();
    if (Playerbots::GetDisabledWithoutRealPlayer() && !HasRealPlayerOnline())
        target = 0;

    while (_activeRandomBotGuids.size() > target)
    {
        ObjectGuid guid = *_activeRandomBotGuids.begin();
        LogoutRandomBot(guid);
    }
}

bool RandomPlayerbotMgr::LoginRosterEntry(RandomBotRosterEntry const& entry)
{
    if (!sBotSessionMgr->LoginReservedCharacter(entry.CharacterGuid, entry.CharacterName))
        return false;

    _activeRandomBotGuids.insert(entry.CharacterGuid);
    return true;
}

void RandomPlayerbotMgr::LogoutRandomBot(ObjectGuid characterGuid)
{
    sBotSessionMgr->LogoutReservedCharacter(characterGuid);
    _activeRandomBotGuids.erase(characterGuid);
}

void RandomPlayerbotMgr::Shutdown()
{
    std::vector<ObjectGuid> toLogout(_activeRandomBotGuids.begin(), _activeRandomBotGuids.end());
    for (ObjectGuid guid : toLogout)
        LogoutRandomBot(guid);
}

bool RandomPlayerbotMgr::IsRandomBot(ObjectGuid characterGuid) const
{
    return _activeRandomBotGuids.contains(characterGuid);
}

void RandomPlayerbotMgr::OnRandomBotLoggedOut(ObjectGuid characterGuid)
{
    _activeRandomBotGuids.erase(characterGuid);
}
