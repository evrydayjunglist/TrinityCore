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

#include "PlayerbotAIBase.h"
#include "PlayerbotsConfig.h"
#include "Player.h"
#include "Packets/MovementPackets.h"
#include "WorldSession.h"
#include <algorithm>

PlayerbotAIBase::PlayerbotAIBase(Player* bot) : _bot(bot)
{
}

bool PlayerbotAIBase::CanUpdateAI() const
{
    return _nextAICheckDelay == 0;
}

void PlayerbotAIBase::UpdateAI(uint32 diff)
{
    // Checked every raw tick, ahead of the react-delay throttle below — mirrors AC's
    // PlayerbotHolder::UpdateSessions(), which polls every bot's teleport state unconditionally
    // rather than on the AI decision cadence, so a stuck bot self-acks as soon as possible.
    if (_bot && _bot->IsBeingTeleported())
    {
        HandleTeleportAck();
        return;
    }

    if (_nextAICheckDelay > diff)
    {
        _nextAICheckDelay -= diff;
        return;
    }

    _nextAICheckDelay = 0;

    if (!CanUpdateAI())
        return;

    UpdateAIInternal(diff);
    SetNextCheckDelay(std::max(Playerbots::GetReactDelay(), MIN_AI_UPDATE_DELAY_MS));
}

void PlayerbotAIBase::SetNextCheckDelay(uint32 delay)
{
    _nextAICheckDelay = delay;
}

void PlayerbotAIBase::HandleTeleportAck()
{
    WorldSession* session = _bot ? _bot->GetSession() : nullptr;
    if (!session || !session->IsBotSession())
        return;

    if (_bot->IsBeingTeleportedFar())
    {
        // Documented in WorldSession.h as safe for server-side calls (already used for the
        // reconnect-mid-teleport edge case) — exactly what a bot's non-existent client would
        // have triggered via CMSG_WORLD_PORT_RESPONSE.
        session->HandleMoveWorldportAck();
        return;
    }

    if (_bot->IsBeingTeleportedNear())
    {
        WorldPacket rawAckPacket(CMSG_MOVE_TELEPORT_ACK);
        WorldPackets::Movement::MoveTeleportAck ack(std::move(rawAckPacket));
        ack.MoverGUID = _bot->GetGUID();
        session->HandleMoveTeleportAck(ack);
    }
}
