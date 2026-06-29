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

#ifndef TRINITY_PLAYERBOT_STRATEGY_H
#define TRINITY_PLAYERBOT_STRATEGY_H

#include "Action.h"
#include "Define.h"
#include "PlayerbotAIAware.h"
#include "Trigger.h"
#include <string>
#include <vector>

class BotPlayerbotAI;

enum StrategyType : uint32
{
    STRATEGY_TYPE_GENERIC = 0,
    STRATEGY_TYPE_COMBAT = 1,
    STRATEGY_TYPE_NONCOMBAT = 2
};

// AC reference: mod-playerbots-master/src/Bot/Engine/Strategy/Strategy.h (minimal port)
class Strategy : public PlayerbotAIAware
{
public:
    explicit Strategy(BotPlayerbotAI* botAI) : PlayerbotAIAware(botAI) { }
    virtual ~Strategy() = default;

    virtual std::vector<NextAction> GetDefaultActions() { return {}; }
    virtual void InitTriggers(std::vector<TriggerNode*>& /*triggers*/) { }
    virtual void InitMultipliers() { }
    virtual std::string GetName() = 0;
    virtual uint32 GetType() const { return STRATEGY_TYPE_GENERIC; }
};

#endif
