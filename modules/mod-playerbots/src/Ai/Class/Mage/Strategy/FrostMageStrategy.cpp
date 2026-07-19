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

#include "FrostMageStrategy.h"

std::vector<NextAction> FrostMageStrategy::GetDefaultActions()
{
    // Filler while a combat target exists but triggers have not boosted a higher cast.
    return { NextAction("frostbolt", 14.0f) };
}

void FrostMageStrategy::InitTriggers(std::vector<TriggerNode*>& triggers)
{
    // Relevance band 18–24: above Gate 12 attack (10–12), below flee (40).
    triggers.push_back(new TriggerNode("has combat target", {
        NextAction("ice lance", 20.0f),
        NextAction("frostbolt", 18.0f)
    }));
}
