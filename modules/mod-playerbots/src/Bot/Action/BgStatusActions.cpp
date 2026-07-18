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

#include "BgStatusActions.h"
#include "BattlegroundPackets.h"
#include "BotPlayerbotAI.h"
#include "Log.h"
#include "Opcodes.h"
#include "Player.h"
#include "PlayerbotsConfig.h"
#include "WorldPacket.h"
#include "WorldSession.h"

bool BgStatusAction::IsUseful()
{
    if (!_botAI)
        return false;

    Player* bot = _botAI->GetBot();
    if (!bot || !bot->GetSession())
        return false;

    return _botAI->GetPendingBgStatus().has_value();
}

bool BgStatusAction::Execute(Event /*event*/)
{
    if (!_botAI)
        return false;

    Player* bot = _botAI->GetBot();
    if (!bot || !bot->GetSession())
        return false;

    std::optional<BotPlayerbotAI::PendingBgStatus> const pending = _botAI->GetPendingBgStatus();
    _botAI->ClearPendingBgStatus();
    if (!pending)
        return false;

    switch (pending->Kind)
    {
        case BotPlayerbotAI::BgStatusKind::NeedConfirmation:
        {
            // AC noise gate: combat/dead → decline (same shape as LfgAcceptAction).
            // Prefer accept otherwise — no ResetStrategies / LeaveBG / era QueuePacket.
            bool const accepted = !(bot->IsInCombat() || !bot->IsAlive());

            WorldPacket packet(CMSG_BATTLEFIELD_PORT);
            WorldPackets::Battleground::BattlefieldPort port(std::move(packet));
            port.Ticket = pending->Ticket;
            port.AcceptedInvite = accepted;
            bot->GetSession()->HandleBattleFieldPortOpcode(port);

            if (Playerbots::GetLogLevel() >= 1)
                TC_LOG_DEBUG("playerbots", "BgStatusAction bot={} kind=NeedConfirmation accepted={} ticketId={} map={}",
                    bot->GetName(),
                    accepted ? "yes" : "no",
                    port.Ticket.Id,
                    pending->Mapid);
            return true;
        }
        case BotPlayerbotAI::BgStatusKind::Queued:
            if (Playerbots::GetLogLevel() >= 1)
                TC_LOG_DEBUG("playerbots", "BgStatusAction bot={} kind=Queued ticketId={} (signal only)",
                    bot->GetName(),
                    pending->Ticket.Id);
            return true;
        case BotPlayerbotAI::BgStatusKind::Active:
            if (Playerbots::GetLogLevel() >= 1)
                TC_LOG_DEBUG("playerbots", "BgStatusAction bot={} kind=Active map={} ticketId={} (signal only)",
                    bot->GetName(),
                    pending->Mapid,
                    pending->Ticket.Id);
            return true;
    }

    return false;
}
