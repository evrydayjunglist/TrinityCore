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

#ifndef TRINITY_PLAYERBOT_EVENT_H
#define TRINITY_PLAYERBOT_EVENT_H

#include <string>

// Minimal Event stub for Gate 6 engine tick path.
class Event
{
public:
    Event() = default;
    explicit Event(std::string source) : _source(std::move(source)) { }

    std::string const& GetSource() const { return _source; }

private:
    std::string _source;
};

#endif
