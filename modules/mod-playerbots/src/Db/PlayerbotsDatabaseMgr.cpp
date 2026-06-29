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

#include "PlayerbotsDatabaseMgr.h"
#include "PlayerbotsConfig.h"
#include "QueryResult.h"

#ifdef WITH_PLAYERBOTS
#include "DatabaseEnv.h"
#include "Implementation/PlayerbotsDatabase.h"
#endif

PlayerbotsDatabaseMgr* PlayerbotsDatabaseMgr::instance()
{
    static PlayerbotsDatabaseMgr instance;
    return &instance;
}

bool PlayerbotsDatabaseMgr::IsConfigured() const
{
    return Playerbots::GetEnableDatabases() && !Playerbots::GetPlayerbotsDatabaseInfo().empty();
}

bool PlayerbotsDatabaseMgr::IsConnected() const
{
#ifdef WITH_PLAYERBOTS
    if (!IsConfigured())
        return false;

    return PlayerbotsDatabase.GetConnectionInfo() != nullptr;
#else
    return false;
#endif
}

bool PlayerbotsDatabaseMgr::CheckVersion(uint32* versionOut) const
{
#ifdef WITH_PLAYERBOTS
    if (!IsConnected())
        return false;

    PreparedQueryResult result = PlayerbotsDatabase.Query(PlayerbotsDatabase.GetPreparedStatement(PLAYERBOTS_SEL_DB_VERSION));
    if (!result)
        return false;

    uint32 const version = (*result)[0].GetUInt32();
    if (versionOut)
        *versionOut = version;
    return true;
#else
    (void)versionOut;
    return false;
#endif
}

std::string PlayerbotsDatabaseMgr::GetStatusSummary() const
{
    if (!Playerbots::GetEnableDatabases())
        return "disabled (Playerbots.Updates.EnableDatabases = 0)";

    if (Playerbots::GetPlayerbotsDatabaseInfo().empty())
        return "not configured (PlayerbotsDatabaseInfo empty)";

#ifdef WITH_PLAYERBOTS
    if (!IsConnected())
        return "not connected";

    uint32 version = 0;
    if (!CheckVersion(&version))
        return "connected, version table missing or empty";

    return "connected, version " + std::to_string(version);
#else
    return "not built (WITH_PLAYERBOTS off)";
#endif
}
