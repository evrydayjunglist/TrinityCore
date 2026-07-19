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

#ifndef TRINITY_PLAYERBOT_VALUE_H
#define TRINITY_PLAYERBOT_VALUE_H

#include "Bot/Engine/PlayerbotAIAware.h"
#include "ObjectGuid.h"
#include "Timer.h"
#include <string>
#include <vector>

class BotPlayerbotAI;
class Player;
class Unit;

// AC-compatible GuidVector alias (mod-playerbots Value.h / ObjectGuidListCalculatedValue).
using GuidVector = std::vector<ObjectGuid>;

inline std::string MakeQualifiedValueName(std::string const& name, std::string const& param)
{
    return name + "::" + param;
}

// AC reference: mod-playerbots-master/src/Bot/Engine/Value/Value.h
// Gate 11: UntypedValue / Value<T> / CalculatedValue<T> with per-interval memoization.
// No PerfMonitor, no MemoryCalculatedValue / LogCalculatedValue (out of starter scope).

class UntypedValue : public PlayerbotAIAware
{
public:
    UntypedValue(BotPlayerbotAI* botAI, std::string name)
        : PlayerbotAIAware(botAI), _name(std::move(name)) { }

    virtual ~UntypedValue() = default;

    virtual void Update() { }
    virtual void Reset() { }

    std::string const& getName() const { return _name; }

protected:
    Player* GetBot() const;

private:
    std::string _name;
};

template <class T>
class Value
{
public:
    virtual ~Value() = default;
    virtual T Get() = 0;
    virtual T LazyGet() = 0;
    virtual void Reset() { }
    virtual void Set(T value) = 0;
};

template <class T>
class CalculatedValue : public UntypedValue, public Value<T>
{
public:
    CalculatedValue(BotPlayerbotAI* botAI, std::string const name = "value", uint32 checkInterval = 1)
        : UntypedValue(botAI, name),
          _checkInterval(checkInterval == 1 ? 1 : (checkInterval < 100 ? checkInterval * 1000 : checkInterval)),
          _lastCheckTime(0),
          _value()
    {
    }

    T Get() override
    {
        if (_checkInterval < 2)
        {
            _value = Calculate();
        }
        else
        {
            uint32 const now = getMSTime();
            if (!_lastCheckTime || getMSTimeDiff(_lastCheckTime, now) >= _checkInterval)
            {
                _lastCheckTime = now;
                _value = Calculate();
            }
        }
        return _value;
    }

    T LazyGet() override
    {
        if (!_lastCheckTime)
            return Get();
        return _value;
    }

    void Set(T val) override { _value = val; }
    void Reset() override { _lastCheckTime = 0; }

protected:
    virtual T Calculate() = 0;

    uint32 _checkInterval;
    uint32 _lastCheckTime;
    T _value;
};

template <class T>
class ManualSetValue : public UntypedValue, public Value<T>
{
public:
    ManualSetValue(BotPlayerbotAI* botAI, T defaultValue, std::string const name = "value")
        : UntypedValue(botAI, name), _value(defaultValue), _defaultValue(defaultValue)
    {
    }

    T Get() override { return _value; }
    T LazyGet() override { return _value; }
    void Set(T val) override { _value = val; }
    void Reset() override { _value = _defaultValue; }

protected:
    T _value;
    T _defaultValue;
};

// AC Qualified — qualifier string for AI_VALUE2 composite keys (name::param).
class Qualified
{
public:
    void Qualify(std::string qualifier) { _qualifier = std::move(qualifier); }
    std::string const& GetQualifier() const { return _qualifier; }

protected:
    std::string _qualifier;
};

#endif
