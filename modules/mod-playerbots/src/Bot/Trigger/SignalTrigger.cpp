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

#include "SignalTrigger.h"
#include "BotPlayerbotAI.h"
#include "Log.h"
#include "Player.h"
#include "PlayerbotsConfig.h"

bool SignalTrigger::IsActive()
{
    if (!_botAI)
        return false;

    if (!_botAI->ConsumeSignal(GetName()))
        return false;

    // Logged so a playtest can confirm the SIGNAL path (not the poll) reacted first (handoff § 8).
    if (Playerbots::GetLogLevel() >= 1)
        TC_LOG_DEBUG("playerbots", "SignalTrigger '{}' fired for bot {}",
            GetName(), _botAI->GetBot() ? _botAI->GetBot()->GetName() : "?");

    return true;
}
