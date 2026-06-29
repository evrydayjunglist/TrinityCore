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

#ifdef WITH_PLAYERBOTS

#include "PlayerbotsDatabase.h"
#include "MySQLPreparedStatement.h"

void PlayerbotsDatabaseConnection::DoPrepareStatements()
{
    if (!m_reconnecting)
        m_stmts.resize(MAX_PLAYERBOTSDATABASE_STATEMENTS);

    PrepareStatement(PLAYERBOTS_SEL_DB_VERSION, "SELECT version FROM playerbots_db_version LIMIT 1", CONNECTION_SYNCH);
}

PlayerbotsDatabaseConnection::PlayerbotsDatabaseConnection(MySQLConnectionInfo& connInfo, ConnectionFlags connectionFlags)
    : MySQLConnection(connInfo, connectionFlags)
{
}

PlayerbotsDatabaseConnection::~PlayerbotsDatabaseConnection() = default;

#endif
