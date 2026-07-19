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

#include "DuelActions.h"
#include "BotPlayerbotAI.h"
#include "DuelPackets.h"
#include "Log.h"
#include "Opcodes.h"
#include "Player.h"
#include "PlayerbotsConfig.h"
#include "WorldPacket.h"
#include "WorldSession.h"

namespace
{
// Master challenger + live CHALLENGED duel. Arbiter from Layer-2 stash, else DuelArbiter UF.
bool ResolveMasterDuelChallenge(BotPlayerbotAI* botAI, Player*& bot, Player*& master, ObjectGuid& arbiterGuid)
{
    if (!botAI)
        return false;

    bot = botAI->GetBot();
    master = botAI->GetMaster();
    if (!bot || !master || !bot->GetSession())
        return false;

    if (!bot->duel || bot->duel->State != DUEL_STATE_CHALLENGED)
        return false;

    // Initiator is the challenger; bot must be the challenged party (not self-observation).
    if (!bot->duel->Initiator || bot->duel->Initiator == bot)
        return false;

    if (bot->duel->Initiator->GetGUID() != master->GetGUID())
        return false;

    arbiterGuid = botAI->GetPendingDuelArbiter();
    if (arbiterGuid.IsEmpty())
        arbiterGuid = *bot->m_playerData->DuelArbiter;

    return !arbiterGuid.IsEmpty();
}
}

bool AcceptDuelAction::IsUseful()
{
    Player* bot = nullptr;
    Player* master = nullptr;
    ObjectGuid arbiterGuid;
    return ResolveMasterDuelChallenge(_botAI, bot, master, arbiterGuid);
}

bool AcceptDuelAction::Execute(Event /*event*/)
{
    Player* bot = nullptr;
    Player* master = nullptr;
    ObjectGuid arbiterGuid;
    if (!ResolveMasterDuelChallenge(_botAI, bot, master, arbiterGuid))
        return false;

    // Midnight: CMSG_DUEL_RESPONSE → HandleDuelResponseOpcode → HandleDuelAccepted.
    // Do not paste AC WotLK CMSG_DUEL_ACCEPTED / HandleDuelAcceptedOpcode / QueuePacket.
    WorldPacket packet(CMSG_DUEL_RESPONSE);
    WorldPackets::Duel::DuelResponse response(std::move(packet));
    response.ArbiterGUID = arbiterGuid;
    response.Accepted = true;
    response.Forfeited = false;
    bot->GetSession()->HandleDuelResponseOpcode(response);

    _botAI->ClearPendingDuelArbiter();

    bool const accepted = bot->duel && bot->duel->State == DUEL_STATE_COUNTDOWN;

    if (Playerbots::GetLogLevel() >= 1)
        TC_LOG_DEBUG("playerbots", "AcceptDuelAction bot={} master={} arbiter={} accepted={}",
            bot->GetName(),
            master->GetName(),
            arbiterGuid.ToString(),
            accepted ? "yes" : "no");

    return accepted;
}
