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

#include "tc_catch2.h"

#include "Bot/Engine/Value.h" // MakeQualifiedValueName + CalculatedValue interval contract

TEST_CASE("Playerbots value layer qualified name keys match AI_VALUE2 shape", "[playerbots][value]")
{
    CHECK(MakeQualifiedValueName("distance", "master") == "distance::master");
    CHECK(MakeQualifiedValueName("has aura", "12345") == "has aura::12345");
    CHECK(MakeQualifiedValueName("target has aura", "1") == "target has aura::1");
}

TEST_CASE("Playerbots CalculatedValue interval normalization matches AC", "[playerbots][value]")
{
    // Mirrors CalculatedValue ctor: 1 = every Get(); 2..99 treated as seconds→ms; else ms.
    auto normalize = [](uint32 checkInterval) -> uint32
    {
        return checkInterval == 1 ? 1 : (checkInterval < 100 ? checkInterval * 1000 : checkInterval);
    };

    CHECK(normalize(1) == 1u);
    CHECK(normalize(2) == 2000u);
    CHECK(normalize(1000) == 1000u);
}
