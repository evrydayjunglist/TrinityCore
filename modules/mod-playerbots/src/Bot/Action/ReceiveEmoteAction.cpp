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

#include "ReceiveEmoteAction.h"
#include "BotPlayerbotAI.h"
#include "Log.h"
#include "Player.h"
#include "PlayerbotsConfig.h"
#include <sstream>

bool ReceiveEmoteAction::IsUseful()
{
    return _botAI && _botAI->GetBot() && _botAI->GetPendingReceiveEmote().has_value();
}

bool ReceiveEmoteAction::Execute(Event /*event*/)
{
    if (!_botAI)
        return false;

    std::optional<BotPlayerbotAI::PendingReceiveEmote> const pending = _botAI->GetPendingReceiveEmote();
    _botAI->ClearPendingReceiveEmote();
    if (!pending)
        return false;

    Player* bot = _botAI->GetBot();
    if (!bot)
        return false;

    // Signal wake-up always succeeds; TellMaster only when source is the master.
    if (!_botAI->HasMaster())
        return true;

    Player* master = _botAI->GetMaster();
    if (!master || pending->SourceGUID != master->GetGUID())
        return true;

    std::ostringstream out;
    if (pending->IsTextEmote)
        out << "saw text emote " << pending->EmoteID;
    else
        out << "saw emote " << pending->EmoteID;

    bool const told = _botAI->TellMaster(out.str());
    if (Playerbots::GetLogLevel() >= 1)
    {
        TC_LOG_DEBUG("playerbots", "ReceiveEmoteAction bot={} master={} text='{}' ok={}",
            bot->GetName(),
            master->GetName(),
            out.str(),
            told ? "yes" : "no");
    }

    return true;
}
