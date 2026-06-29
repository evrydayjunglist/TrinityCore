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

#ifndef TRINITY_PLAYERBOT_TRIGGER_H
#define TRINITY_PLAYERBOT_TRIGGER_H

#include "Action.h"
#include "PlayerbotAIAware.h"
#include <string>
#include <vector>

class BotPlayerbotAI;

// AC reference: mod-playerbots-master/src/Bot/Engine/Trigger/Trigger.h (minimal port)
class Trigger : public PlayerbotAIAware
{
public:
    Trigger(BotPlayerbotAI* botAI, std::string name = "trigger")
        : PlayerbotAIAware(botAI), _name(std::move(name)) { }

    virtual ~Trigger() = default;

    virtual bool IsActive() { return false; }
    virtual std::vector<NextAction> GetHandlers() { return {}; }

    std::string const& GetName() const { return _name; }

private:
    std::string _name;
};

class TriggerNode
{
public:
    TriggerNode(std::string name, std::vector<NextAction> handlers = {})
        : _name(std::move(name)), _handlers(std::move(handlers)), _trigger(nullptr) { }

    Trigger* GetTrigger() const { return _trigger; }
    void SetTrigger(Trigger* trigger) { _trigger = trigger; }
    std::string const& GetName() const { return _name; }

    std::vector<NextAction> GetHandlers() const
    {
        std::vector<NextAction> result = _handlers;
        if (_trigger)
        {
            std::vector<NextAction> extra = _trigger->GetHandlers();
            result.insert(result.end(), extra.begin(), extra.end());
        }
        return result;
    }

private:
    std::string _name;
    std::vector<NextAction> _handlers;
    Trigger* _trigger;
};

#endif
