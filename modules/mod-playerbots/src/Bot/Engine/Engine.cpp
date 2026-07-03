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

#include "Engine.h"
#include "Action.h"
#include "AiObjectContext.h"
#include "BotPlayerbotAI.h"
#include "Event.h"
#include "Log.h"
#include "Player.h"
#include "PlayerbotsConfig.h"
#include <algorithm>

namespace
{
constexpr uint32 DEBUG_LOG_INTERVAL_MS = 5000;
}

Engine::Engine(BotPlayerbotAI* botAI, AiObjectContext* context)
    : PlayerbotAIAware(botAI), _context(context)
{
}

void Engine::Init()
{
    for (TriggerNode* trigger : _triggers)
        delete trigger;

    _triggers.clear();

    for (auto const& [name, strategy] : _strategies)
    {
        if (!strategy)
            continue;

        strategy->InitTriggers(_triggers);
        strategy->InitMultipliers();

        if (Playerbots::GetLogLevel() >= 1)
        {
            Player* bot = _botAI ? _botAI->GetBot() : nullptr;
            TC_LOG_DEBUG("playerbots", "Engine::Init strategy '{}' for bot {}",
                name, bot ? bot->GetName() : "?");
        }
    }

    // Resolve each TriggerNode's name to the registered Trigger object (AC: Engine::Init does
    // the same context lookup) — a node whose name is unknown can never fire, so flag it loudly.
    if (_context)
    {
        for (TriggerNode* node : _triggers)
        {
            if (!node)
                continue;

            Trigger* trigger = _context->GetTrigger(node->GetName());
            if (!trigger)
            {
                TC_LOG_ERROR("playerbots", "Engine::Init unknown trigger '{}'", node->GetName());
                continue;
            }

            node->SetTrigger(trigger);
        }
    }
}

void Engine::AddStrategy(std::string const& name)
{
    if (!_context)
        return;

    Strategy* strategy = _context->GetStrategy(name);
    if (!strategy)
    {
        TC_LOG_ERROR("playerbots", "Engine::AddStrategy unknown strategy '{}'", name);
        return;
    }

    _strategies[name] = strategy;
}

void Engine::RemoveAllStrategies()
{
    _strategies.clear();
}

bool Engine::HasStrategy(std::string const& name) const
{
    return _strategies.find(name) != _strategies.end();
}

Strategy* Engine::GetStrategy(std::string const& name) const
{
    auto itr = _strategies.find(name);
    return itr != _strategies.end() ? itr->second : nullptr;
}

std::vector<std::string> Engine::GetStrategyNames() const
{
    std::vector<std::string> names;
    names.reserve(_strategies.size());
    for (auto const& [name, strategy] : _strategies)
    {
        if (strategy)
            names.push_back(name);
    }
    return names;
}

void Engine::ProcessTriggers(std::vector<NextAction>& actions)
{
    for (TriggerNode* node : _triggers)
    {
        if (!node)
            continue;

        Trigger* trigger = node->GetTrigger();
        if (!trigger || !trigger->IsActive())
            continue;

        std::vector<NextAction> const handlers = node->GetHandlers();
        actions.insert(actions.end(), handlers.begin(), handlers.end());
    }
}

void Engine::PushDefaultActions(std::vector<NextAction>& actions)
{
    for (auto const& [name, strategy] : _strategies)
    {
        if (!strategy)
            continue;

        std::vector<NextAction> const defaults = strategy->GetDefaultActions();
        actions.insert(actions.end(), defaults.begin(), defaults.end());
    }
}

void Engine::LogTickThrottled(uint32 diff)
{
    if (Playerbots::GetLogLevel() < 1)
        return;

    _lastDebugLogMs += diff;
    if (_lastDebugLogMs < DEBUG_LOG_INTERVAL_MS)
        return;

    _lastDebugLogMs = 0;

    std::string strategyList;
    for (std::string const& name : GetStrategyNames())
    {
        if (!strategyList.empty())
            strategyList += ',';
        strategyList += name;
    }

    Player* bot = _botAI ? _botAI->GetBot() : nullptr;
    TC_LOG_DEBUG("playerbots", "Engine tick bot={} strategies={}",
        bot ? bot->GetName() : "?",
        strategyList.empty() ? "none" : strategyList);
}

bool Engine::DoNextAction(uint32 diff)
{
    if (_gcdRemaining > diff)
    {
        _gcdRemaining -= diff;
        LogTickThrottled(diff);
        return false;
    }

    _gcdRemaining = 0;

    if (!_context)
    {
        LogTickThrottled(diff);
        return false;
    }

    std::vector<NextAction> candidates;
    ProcessTriggers(candidates);
    PushDefaultActions(candidates);

    std::ranges::sort(candidates, [](NextAction const& a, NextAction const& b)
    {
        return a.GetRelevance() > b.GetRelevance();
    });

    Event event;
    for (NextAction const& candidate : candidates)
    {
        Action* action = _context->GetAction(candidate.GetName());
        if (!action || !action->IsPossible() || !action->IsUseful())
            continue;

        if (action->Execute(event))
        {
            if (Playerbots::GetLogLevel() >= 1)
            {
                Player* bot = _botAI ? _botAI->GetBot() : nullptr;
                TC_LOG_DEBUG("playerbots", "Engine executed action '{}' for bot {}",
                    candidate.GetName(), bot ? bot->GetName() : "?");
            }

            _gcdRemaining = Playerbots::GetGlobalCooldown();
            LogTickThrottled(diff);
            return true;
        }
    }

    LogTickThrottled(diff);
    return false;
}
