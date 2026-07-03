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

#ifndef TRINITY_PLAYERBOT_AI_OBJECT_CONTEXT_H
#define TRINITY_PLAYERBOT_AI_OBJECT_CONTEXT_H

#include "Action.h"
#include "PlayerbotAIAware.h"
#include "Strategy.h"
#include "Trigger.h"
#include <map>
#include <memory>
#include <string>

class BotPlayerbotAI;

// AC reference: mod-playerbots-master/src/Bot/Engine/AiObjectContext.h (stub — Gate 6 passive only;
// trigger registry added in Gate 10b so strategies' TriggerNodes resolve to real Trigger objects,
// same name-keyed lookup AC's context provides)
class AiObjectContext : public PlayerbotAIAware
{
public:
    explicit AiObjectContext(BotPlayerbotAI* botAI);
    ~AiObjectContext() = default;

    void RegisterStrategy(std::string const& name, std::unique_ptr<Strategy> strategy);
    Strategy* GetStrategy(std::string const& name);

    void RegisterAction(std::string const& name, std::unique_ptr<Action> action);
    Action* GetAction(std::string const& name);

    void RegisterTrigger(std::string const& name, std::unique_ptr<Trigger> trigger);
    Trigger* GetTrigger(std::string const& name);

private:
    std::map<std::string, std::unique_ptr<Strategy>> _strategies;
    std::map<std::string, std::unique_ptr<Action>> _actions;
    std::map<std::string, std::unique_ptr<Trigger>> _triggers;
};

#endif
