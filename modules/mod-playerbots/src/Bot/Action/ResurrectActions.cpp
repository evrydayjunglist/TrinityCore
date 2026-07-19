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

#include "ResurrectActions.h"
#include "BotPlayerbotAI.h"
#include "Log.h"
#include "MiscPackets.h"
#include "Opcodes.h"
#include "Player.h"
#include "PlayerbotsConfig.h"
#include "WorldPacket.h"
#include "WorldSession.h"

namespace
{
bool HasPendingResurrect(BotPlayerbotAI* botAI, Player*& bot, ObjectGuid& offerer)
{
    if (!botAI)
        return false;

    bot = botAI->GetBot();
    if (!bot || bot->IsAlive() || !bot->IsResurrectRequested())
        return false;

    offerer = bot->GetResurrectRequesterGUID();
    return !offerer.IsEmpty() && bot->IsResurrectRequestedBy(offerer);
}
}

bool AcceptResurrectAction::IsUseful()
{
    Player* bot = nullptr;
    ObjectGuid offerer;
    return HasPendingResurrect(_botAI, bot, offerer);
}

bool AcceptResurrectAction::Execute(Event /*event*/)
{
    Player* bot = nullptr;
    ObjectGuid offerer;
    if (!HasPendingResurrect(_botAI, bot, offerer))
        return false;

    // Midnight polarity: Response == 0 accept (MiscHandler::HandleResurrectResponse).
    // Do not paste AC WotLK AcceptResurrectAction's uint8(1).
    WorldPacket packet(CMSG_RESURRECT_RESPONSE);
    WorldPackets::Misc::ResurrectResponse response(std::move(packet));
    response.Resurrecter = offerer;
    response.Response = 0;
    bot->GetSession()->HandleResurrectResponse(response);

    // ResurrectUsingRequestData teleports to the offerer first; when that sets
    // IsBeingTeleported(), the actual ResurrectPlayer runs as DELAYED_RESURRECT_PLAYER
    // after the teleport — so IsAlive() is still false on this tick.
    bool const accepted = bot->IsAlive() || bot->IsBeingTeleported();

    if (Playerbots::GetLogLevel() >= 1)
        TC_LOG_DEBUG("playerbots", "AcceptResurrectAction bot={} offerer={} accepted={}",
            bot->GetName(), offerer.ToString(), accepted ? "yes" : "no");

    return accepted;
}
