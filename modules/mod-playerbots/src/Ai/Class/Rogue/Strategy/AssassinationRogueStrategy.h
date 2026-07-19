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

#ifndef TRINITY_PLAYERBOT_ASSASSINATION_ROGUE_STRATEGY_H
#define TRINITY_PLAYERBOT_ASSASSINATION_ROGUE_STRATEGY_H

#include "Strategy.h"

class BotPlayerbotAI;

// Gate 14 pilot combat rotation — opener / filler / spender. Flee (40) stays above these.
class AssassinationRogueStrategy : public Strategy
{
public:
    explicit AssassinationRogueStrategy(BotPlayerbotAI* botAI) : Strategy(botAI) { }

    std::string GetName() override { return "assassination"; }
    uint32 GetType() const override { return STRATEGY_TYPE_COMBAT; }
    std::vector<NextAction> GetDefaultActions() override;
    void InitTriggers(std::vector<TriggerNode*>& triggers) override;
};

#endif
