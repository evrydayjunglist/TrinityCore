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

#include "XpGainAction.h"
#include "BotPlayerbotAI.h"
#include "Log.h"
#include "Player.h"
#include "PlayerbotsConfig.h"
#include <sstream>

bool XpGainAction::IsUseful()
{
    return _botAI && _botAI->GetBot() && _botAI->GetPendingXpGain().has_value();
}

bool XpGainAction::Execute(Event /*event*/)
{
    if (!_botAI)
        return false;

    std::optional<BotPlayerbotAI::PendingXpGain> const pending = _botAI->GetPendingXpGain();
    _botAI->ClearPendingXpGain();
    if (!pending)
        return false;

    Player* bot = _botAI->GetBot();
    if (!bot)
        return false;

    // Signal wake-up always succeeds; TellMaster only when master present.
    if (!_botAI->HasMaster())
        return true;

    std::ostringstream out;
    out << "gained " << pending->Amount << " xp";
    bool const told = _botAI->TellMaster(out.str());
    if (Playerbots::GetLogLevel() >= 1)
    {
        Player* master = _botAI->GetMaster();
        TC_LOG_DEBUG("playerbots", "XpGainAction bot={} master={} text='{}' ok={}",
            bot->GetName(),
            master ? master->GetName() : "none",
            out.str(),
            told ? "yes" : "no");
    }

    return true;
}
