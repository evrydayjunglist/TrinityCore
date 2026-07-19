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
#include "Value.h"
#include <functional>
#include <map>
#include <memory>
#include <sstream>
#include <string>

class BotPlayerbotAI;

// AC reference: mod-playerbots-master/src/Bot/Engine/AiObjectContext.h
// Fork keeps unique_ptr maps (Gate 6) and adds a 4th value registry (Gate 11).
// Qualified AI_VALUE2 keys use lazy creators (name::param) instead of AC NamedObjectContext.
class AiObjectContext : public PlayerbotAIAware
{
public:
    using QualifiedValueCreator =
        std::function<std::unique_ptr<UntypedValue>(BotPlayerbotAI* botAI, std::string const& qualifier)>;

    explicit AiObjectContext(BotPlayerbotAI* botAI);
    ~AiObjectContext() = default;

    void RegisterStrategy(std::string const& name, std::unique_ptr<Strategy> strategy);
    Strategy* GetStrategy(std::string const& name);

    void RegisterAction(std::string const& name, std::unique_ptr<Action> action);
    Action* GetAction(std::string const& name);

    void RegisterTrigger(std::string const& name, std::unique_ptr<Trigger> trigger);
    Trigger* GetTrigger(std::string const& name);

    void RegisterValue(std::string const& name, std::unique_ptr<UntypedValue> value);
    void RegisterQualifiedValueCreator(std::string const& name, QualifiedValueCreator creator);

    UntypedValue* GetUntypedValue(std::string const& name);

    template <class T>
    Value<T>* GetValue(std::string const& name)
    {
        return dynamic_cast<Value<T>*>(GetUntypedValue(name));
    }

    template <class T>
    Value<T>* GetValue(std::string const& name, std::string const& param)
    {
        return GetValue<T>(MakeQualifiedValueName(name, param));
    }

    template <class T>
    Value<T>* GetValue(std::string const& name, int32 param)
    {
        std::ostringstream out;
        out << param;
        return GetValue<T>(name, out.str());
    }

private:
    UntypedValue* CreateQualifiedValue(std::string const& qualifiedName);

    std::map<std::string, std::unique_ptr<Strategy>> _strategies;
    std::map<std::string, std::unique_ptr<Action>> _actions;
    std::map<std::string, std::unique_ptr<Trigger>> _triggers;
    std::map<std::string, std::unique_ptr<UntypedValue>> _values;
    std::map<std::string, QualifiedValueCreator> _qualifiedValueCreators;
};

#endif
