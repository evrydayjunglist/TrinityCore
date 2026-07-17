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

#include "TellMasterActions.h"
#include "BotPlayerbotAI.h"
#include "Log.h"
#include "Player.h"
#include "PlayerbotsConfig.h"

bool TellMasterAction::IsUseful()
{
    return _botAI && _botAI->HasMaster() && _botAI->GetBot();
}

bool TellMasterAction::Execute(Event /*event*/)
{
    if (!_botAI)
        return false;

    bool const told = _botAI->TellMaster(_text);
    if (Playerbots::GetLogLevel() >= 1)
    {
        Player* bot = _botAI->GetBot();
        Player* master = _botAI->GetMaster();
        TC_LOG_DEBUG("playerbots", "TellMasterAction bot={} master={} text='{}' ok={}",
            bot ? bot->GetName() : "?",
            master ? master->GetName() : "none",
            _text,
            told ? "yes" : "no");
    }

    return told;
}
