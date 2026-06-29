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

#ifndef TRINITY_BOT_PLAYERBOT_AI_H
#define TRINITY_BOT_PLAYERBOT_AI_H

#include "AiObjectContext.h"
#include "Engine.h"
#include "PlayerbotAIBase.h"
#include <memory>

class AiObjectContext;
class Engine;

// Gate 6: bot AI with AC-shaped Engine + passive strategy.
class BotPlayerbotAI : public PlayerbotAIBase
{
public:
    explicit BotPlayerbotAI(Player* bot);
    ~BotPlayerbotAI() = default;

    void ResetStrategies();
    Engine* GetEngine() const { return _engine.get(); }
    AiObjectContext* GetAiObjectContext() const { return _context.get(); }

    Player* GetMaster() const { return _master; }
    void SetMaster(Player* master) { _master = master; }
    bool HasMaster() const { return _master != nullptr; }

protected:
    void UpdateAIInternal(uint32 diff) override;

private:
    std::unique_ptr<AiObjectContext> _context;
    std::unique_ptr<Engine> _engine;
    Player* _master = nullptr;
};

#endif
