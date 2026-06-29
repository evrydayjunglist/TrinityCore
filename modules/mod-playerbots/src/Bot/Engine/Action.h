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

#ifndef TRINITY_PLAYERBOT_ACTION_H
#define TRINITY_PLAYERBOT_ACTION_H

#include "PlayerbotAIAware.h"
#include "Event.h"
#include <string>
#include <vector>

class BotPlayerbotAI;
class Player;

enum ActionResult
{
    ACTION_RESULT_UNKNOWN,
    ACTION_RESULT_OK,
    ACTION_RESULT_IMPOSSIBLE,
    ACTION_RESULT_USELESS,
    ACTION_RESULT_FAILED
};

class NextAction
{
public:
    NextAction(std::string name, float relevance = 0.0f) : _relevance(relevance), _name(std::move(name)) { }

    std::string const& GetName() const { return _name; }
    float GetRelevance() const { return _relevance; }

private:
    float _relevance;
    std::string _name;
};

// AC reference: mod-playerbots-master/src/Bot/Engine/Action/Action.h (minimal port)
class Action : public PlayerbotAIAware
{
public:
    Action(BotPlayerbotAI* botAI, std::string name = "action")
        : PlayerbotAIAware(botAI), _name(std::move(name)) { }

    virtual ~Action() = default;

    virtual bool Execute(Event /*event*/) { return true; }
    virtual bool IsUseful() { return true; }
    virtual bool IsPossible() { return true; }

    std::string const& GetName() const { return _name; }

protected:
    Player* GetBot() const;

private:
    std::string _name;
};

#endif
