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

#ifndef TRINITY_PLAYERBOT_ENGINE_H
#define TRINITY_PLAYERBOT_ENGINE_H

#include "Define.h"
#include "PlayerbotAIAware.h"
#include "Strategy.h"
#include "Trigger.h"
#include <map>
#include <string>
#include <vector>

class AiObjectContext;
class BotPlayerbotAI;

// AC reference: mod-playerbots-master/src/Bot/Engine/Engine.h (minimal port)
class Engine : public PlayerbotAIAware
{
public:
    Engine(BotPlayerbotAI* botAI, AiObjectContext* context);

    void Init();
    void AddStrategy(std::string const& name);
    void RemoveAllStrategies();
    bool HasStrategy(std::string const& name) const;
    Strategy* GetStrategy(std::string const& name) const;
    std::vector<std::string> GetStrategyNames() const;

    // Returns true if an action executed. triggersProcessed is true when ProcessTriggers ran
    // this call (false while on global cooldown — caller must not burn pending-signal TTL then).
    bool DoNextAction(uint32 diff, bool& triggersProcessed);

private:
    void ProcessTriggers(std::vector<NextAction>& actions);
    void PushDefaultActions(std::vector<NextAction>& actions);
    void LogTickThrottled(uint32 diff);

    AiObjectContext* _context;
    std::map<std::string, Strategy*> _strategies;
    std::vector<TriggerNode*> _triggers;
    uint32 _gcdRemaining = 0;
    uint32 _lastDebugLogMs = 0;
};

#endif
