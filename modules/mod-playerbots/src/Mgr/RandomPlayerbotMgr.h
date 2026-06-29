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

#ifndef TRINITY_RANDOM_PLAYERBOT_MGR_H
#define TRINITY_RANDOM_PLAYERBOT_MGR_H

#include "ObjectGuid.h"
#include <string>
#include <unordered_set>
#include <vector>

struct RandomBotRosterEntry
{
    uint32 AccountId = 0;
    std::string CharacterName;
    ObjectGuid CharacterGuid;
};

class RandomPlayerbotMgr
{
public:
    static RandomPlayerbotMgr* instance();

    void Init();
    void Update(uint32 diff);
    void Shutdown();

    bool IsRandomBot(ObjectGuid characterGuid) const;
    size_t GetActiveRandomBotCount() const { return _activeRandomBotGuids.size(); }
    size_t GetTargetRandomBotCount() const;
    bool IsSchedulerEnabled() const;
    void SetSchedulerPaused(bool paused) { _schedulerPaused = paused; }
    bool IsSchedulerPaused() const { return _schedulerPaused; }

    void TriggerSchedulerPass();
    void OnRandomBotLoggedOut(ObjectGuid characterGuid);

private:
    RandomPlayerbotMgr() = default;

    bool HasRealPlayerOnline() const;
    void BuildRoster();
    void TryLoginRandomBots();
    void TryLogoutExcessRandomBots();
    bool LoginRosterEntry(RandomBotRosterEntry const& entry);
    void LogoutRandomBot(ObjectGuid characterGuid);

    std::vector<RandomBotRosterEntry> _roster;
    std::unordered_set<ObjectGuid> _activeRandomBotGuids;
    uint32 _updateTimer = 0;
    bool _schedulerPaused = false;
};

#define sRandomPlayerbotMgr RandomPlayerbotMgr::instance()

#endif
