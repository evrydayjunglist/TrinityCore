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

#include "AiObjectContext.h"

AiObjectContext::AiObjectContext(BotPlayerbotAI* botAI) : PlayerbotAIAware(botAI) { }

void AiObjectContext::RegisterStrategy(std::string const& name, std::unique_ptr<Strategy> strategy)
{
    _strategies[name] = std::move(strategy);
}

Strategy* AiObjectContext::GetStrategy(std::string const& name)
{
    auto itr = _strategies.find(name);
    return itr != _strategies.end() ? itr->second.get() : nullptr;
}

void AiObjectContext::RegisterAction(std::string const& name, std::unique_ptr<Action> action)
{
    _actions[name] = std::move(action);
}

Action* AiObjectContext::GetAction(std::string const& name)
{
    auto itr = _actions.find(name);
    return itr != _actions.end() ? itr->second.get() : nullptr;
}

void AiObjectContext::RegisterTrigger(std::string const& name, std::unique_ptr<Trigger> trigger)
{
    _triggers[name] = std::move(trigger);
}

Trigger* AiObjectContext::GetTrigger(std::string const& name)
{
    auto itr = _triggers.find(name);
    return itr != _triggers.end() ? itr->second.get() : nullptr;
}
