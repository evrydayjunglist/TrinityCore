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

#ifndef TRINITY_PLAYERBOT_COMBAT_STRATEGY_H
#define TRINITY_PLAYERBOT_COMBAT_STRATEGY_H

#include "Strategy.h"

class BotPlayerbotAI;

// AC reference: mod-playerbots-master/src/Ai/Base/Strategy/CombatStrategy.h (minimal Gate 8)
class CombatStrategy : public Strategy
{
public:
    explicit CombatStrategy(BotPlayerbotAI* botAI) : Strategy(botAI) { }

    std::string GetName() override { return "attack"; }
    uint32 GetType() const override { return STRATEGY_TYPE_COMBAT; }
    std::vector<NextAction> GetDefaultActions() override;
};

#endif
