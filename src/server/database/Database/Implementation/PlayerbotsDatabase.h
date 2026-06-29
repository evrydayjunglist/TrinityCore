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

#ifndef TRINITYCORE_PLAYERBOTS_DATABASE_H
#define TRINITYCORE_PLAYERBOTS_DATABASE_H

#include "MySQLConnection.h"

enum PlayerbotsDatabaseStatements : uint32
{
    PLAYERBOTS_SEL_DB_VERSION,

    MAX_PLAYERBOTSDATABASE_STATEMENTS
};

class TC_DATABASE_API PlayerbotsDatabaseConnection : public MySQLConnection
{
public:
    typedef PlayerbotsDatabaseStatements Statements;

    PlayerbotsDatabaseConnection(MySQLConnectionInfo& connInfo, ConnectionFlags connectionFlags);
    ~PlayerbotsDatabaseConnection();

    void DoPrepareStatements() override;
};

#endif

#endif
