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
#include "DatabaseEnv.h"
#include "Log.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "PlayerbotsConfig.h"
#include "PlayerbotsDatabaseMgr.h"
#include "RandomPlayerbotFactory.h"
#include "World.h"
#include "WorldSession.h"

#include <chrono>
#include <set>
#include <thread>

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

    // Phase A/B: provision reserved bot accounts + level-1 characters (or run the teardown switch).
    // Idempotent and a no-op unless the random-bot feature is enabled. Returns how many NEW
    // characters were created this run.
    uint32 const generated = RandomPlayerbotFactory::GenerateRandomBots();

    if (!IsSchedulerEnabled())
        return;

    if (Playerbots::GetEnableDatabases() && !sPlayerbotsDatabaseMgr->CheckVersion())
        TC_LOG_WARN("server.loading", "Playerbots: random bot DB enabled but version check failed — scheduler may be limited.");

    // Player::SaveToDB (used by generation) enqueues an ASYNC CharacterDatabase commit, so a
    // synchronous read issued right after — BuildRoster's SELECT — races the uncommitted INSERTs
    // and under-counts the roster (and a subsequent login would fail to load the char). When we
    // just generated characters, drain the async queue first, exactly as AC's RandomPlayerbotFactory
    // does before its dependent reads. Bounded so a stuck queue can't hang startup; logged either
    // way (never a silent cap).
    if (generated > 0)
    {
        uint32 const maxWaitMs = 30000;
        uint32 waited = 0;
        while (CharacterDatabase.QueueSize() > 0 && waited < maxWaitMs)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            waited += 50;
        }
        // QueueSize() == 0 means the last op has been dequeued; a short final sleep lets the worker
        // finish executing it (AC uses the same 100 ms margin).
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        if (CharacterDatabase.QueueSize() > 0)
            TC_LOG_WARN("server.loading", "Playerbots: CharacterDatabase still draining after {}ms while building the random-bot roster ({} char(s) just generated) — roster may under-count until next restart.", maxWaitMs, generated);
        else
            TC_LOG_INFO("server.loading", "Playerbots: drained CharacterDatabase queue in ~{}ms after generating {} bot character(s) — building roster.", waited, generated);
    }

    BuildRoster();
}

void RandomPlayerbotMgr::BuildRoster()
{
    _roster.clear();

    // AC-style DB enumeration (RandomPlayerbotFactory model): the roster is every non-deleted
    // character living on a reserved bot account, across all reservation modes (name prefix,
    // explicit ids, id range). This supersedes the deprecated RandomBotAccounts /
    // RandomBotCharacterNames hand-list. See playerbots-random-bot-generation-handoff.md § 5.
    std::set<uint32> botAccountIds;

    std::string const prefix = Playerbots::GetRandomBotAccountPrefix();
    if (!prefix.empty())
    {
        if (QueryResult result = LoginDatabase.PQuery("SELECT id FROM account WHERE username LIKE '{}%'", prefix))
        {
            do
            {
                botAccountIds.insert(result->Fetch()[0].GetUInt32());
            } while (result->NextRow());
        }
    }

    for (uint32 id : Playerbots::GetReservedAccountIds())
        botAccountIds.insert(id);

    uint32 const minId = Playerbots::GetReservedAccountMinId();
    uint32 const maxId = Playerbots::GetReservedAccountMaxId();
    if (minId != 0 && maxId != 0 && minId <= maxId)
    {
        if (QueryResult result = LoginDatabase.PQuery("SELECT id FROM account WHERE id BETWEEN {} AND {}", minId, maxId))
        {
            do
            {
                botAccountIds.insert(result->Fetch()[0].GetUInt32());
            } while (result->NextRow());
        }
    }

    if (botAccountIds.empty())
    {
        TC_LOG_INFO("server.loading", "Playerbots: no reserved bot accounts configured — random bot roster is empty.");
        return;
    }

    for (uint32 accountId : botAccountIds)
    {
        QueryResult result = CharacterDatabase.PQuery(
            "SELECT guid, name FROM characters WHERE account = {} AND deleteInfos_Name IS NULL", accountId);
        if (!result)
            continue;

        do
        {
            Field* fields = result->Fetch();

            RandomBotRosterEntry entry;
            entry.AccountId = accountId;
            entry.CharacterGuid = ObjectGuid::Create<HighGuid::Player>(fields[0].GetUInt64());
            entry.CharacterName = fields[1].GetString();
            _roster.push_back(std::move(entry));
        } while (result->NextRow());
    }

    TC_LOG_INFO("server.loading", "Playerbots: random bot roster built — {} character(s) across {} reserved account(s).",
        _roster.size(), botAccountIds.size());
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
