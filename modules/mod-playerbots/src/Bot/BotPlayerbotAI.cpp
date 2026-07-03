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

#include "BotPlayerbotAI.h"
#include "AiFactory.h"
#include "Engine.h"
#include "Log.h"
#include "Player.h"
#include "PlayerbotsConfig.h"
#include "Random.h"
#include "RandomPlayerbotMgr.h"

BotPlayerbotAI::BotPlayerbotAI(Player* bot) : PlayerbotAIBase(bot)
{
    _context = AiFactory::CreateContext(this, bot);
    _engine = std::make_unique<Engine>(this, _context.get());
    ResetStrategies();
}

void BotPlayerbotAI::ResetStrategies()
{
    if (!_engine)
        return;

    _engine->RemoveAllStrategies();

    std::string appliedStrategies;
    if (HasMaster())
    {
        _engine->AddStrategy("follow");
        _engine->AddStrategy("attack");
        appliedStrategies = "follow,attack";
    }
    else if (Player* bot = GetBot(); bot && sRandomPlayerbotMgr->IsRandomBot(bot->GetGUID()) &&
        roll_chance(Playerbots::GetRandomBotRpgChance()))
    {
        // AC: masterless random bots run the "newrpg" wander/grind/quest-giver loop instead of
        // sitting passive; RandomBotRpgChance is this fork's single on/off-with-a-dial knob for it.
        _engine->AddStrategy("newrpg");
        appliedStrategies = "newrpg";
        _rpgInfo.Reset(); // Gate 10b: fresh state machine whenever the RPG strategy is (re)applied
    }
    else
    {
        _engine->AddStrategy("passive");
        appliedStrategies = "passive";
    }

    _engine->Init();

    if (Playerbots::GetLogLevel() >= 1)
    {
        TC_LOG_DEBUG("playerbots", "BotPlayerbotAI::ResetStrategies bot={} master={} strategies={}",
            GetBot() ? GetBot()->GetName() : "?",
            HasMaster() ? "yes" : "no",
            appliedStrategies);
    }
}

void BotPlayerbotAI::UpdateAIInternal(uint32 diff)
{
    if (_engine)
        _engine->DoNextAction(diff);
}
