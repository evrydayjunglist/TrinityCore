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

#include "BeastMasteryHunterStrategy.h"

std::vector<NextAction> BeastMasteryHunterStrategy::GetDefaultActions()
{
    return { NextAction("cobra shot", 14.0f) };
}

void BeastMasteryHunterStrategy::InitTriggers(std::vector<TriggerNode*>& triggers)
{
    // Relevance band 18–24: above Gate 12 attack (10–12), below flee (40).
    triggers.push_back(new TriggerNode("has combat target", {
        NextAction("kill command", 24.0f),
        NextAction("barbed shot", 22.0f),
        NextAction("cobra shot", 18.0f)
    }));
}
