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

#ifndef TRINITY_PLAYERBOT_FLEE_STRATEGY_H
#define TRINITY_PLAYERBOT_FLEE_STRATEGY_H

#include "Strategy.h"

class BotPlayerbotAI;

// Gate 12 — AC-shaped +flee: low-health flee outranks attack/follow when active.
class FleeStrategy : public Strategy
{
public:
    explicit FleeStrategy(BotPlayerbotAI* botAI) : Strategy(botAI) { }

    std::string GetName() override { return "flee"; }
    uint32 GetType() const override { return STRATEGY_TYPE_COMBAT; }
    void InitTriggers(std::vector<TriggerNode*>& triggers) override;
};

#endif
