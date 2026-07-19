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

#include "AssassinationRogueStrategy.h"

std::vector<NextAction> AssassinationRogueStrategy::GetDefaultActions()
{
    // Filler while a combat target exists but triggers have not boosted a higher cast.
    // Below flee (40) and combat attack boost (12); above idle follow (1).
    return { NextAction("mutilate", 14.0f) };
}

void AssassinationRogueStrategy::InitTriggers(std::vector<TriggerNode*>& triggers)
{
    // Relevance band 18–24: above Gate 12 attack (10–12), below flee (40).
    triggers.push_back(new TriggerNode("has combat target", {
        NextAction("garrote", 24.0f),
        NextAction("envenom", 22.0f),
        NextAction("mutilate", 18.0f)
    }));
}
