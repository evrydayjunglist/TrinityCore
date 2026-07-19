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

#include "LfgActions.h"
#include "BotPlayerbotAI.h"
#include "Group.h"
#include "LFG.h"
#include "LFGMgr.h"
#include "LFGPackets.h"
#include "Log.h"
#include "Opcodes.h"
#include "Player.h"
#include "PlayerbotsConfig.h"
#include "SharedDefines.h"
#include "WorldPacket.h"
#include "WorldSession.h"

namespace
{
// V1 thin class→role subset (no AC GetRoles / spec-tab fidelity).
// Tank classes → TANK; Priest → HEALER; everything else → DAMAGE.
uint8 ThinLfgRolesDesired(Player const* bot)
{
    if (!bot)
        return lfg::PLAYER_ROLE_DAMAGE;

    switch (bot->GetClass())
    {
        case CLASS_WARRIOR:
        case CLASS_DEATH_KNIGHT:
        case CLASS_DEMON_HUNTER:
            return lfg::PLAYER_ROLE_TANK;
        case CLASS_PRIEST:
            return lfg::PLAYER_ROLE_HEALER;
        default:
            return lfg::PLAYER_ROLE_DAMAGE;
    }
}
}

bool LfgRoleCheckAction::IsUseful()
{
    if (!_botAI)
        return false;

    Player* bot = _botAI->GetBot();
    if (!bot || !bot->GetSession() || !bot->GetGroup())
        return false;

    return sLFGMgr->GetState(bot->GetGUID()) == lfg::LFG_STATE_ROLECHECK;
}

bool LfgRoleCheckAction::Execute(Event /*event*/)
{
    if (!_botAI)
        return false;

    Player* bot = _botAI->GetBot();
    if (!bot || !bot->GetSession() || !bot->GetGroup())
        return false;

    if (sLFGMgr->GetState(bot->GetGUID()) != lfg::LFG_STATE_ROLECHECK)
        return false;

    // Midnight: CMSG_DF_SET_ROLES → HandleLfgSetRolesOpcode.
    // Do not paste AC WotLK CMSG_LFG_SET_ROLES / QueuePacket.
    WorldPacket packet(CMSG_DF_SET_ROLES);
    WorldPackets::LFG::DFSetRoles setRoles(std::move(packet));
    setRoles.RolesDesired = ThinLfgRolesDesired(bot);
    bot->GetSession()->HandleLfgSetRolesOpcode(setRoles);

    if (Playerbots::GetLogLevel() >= 1)
        TC_LOG_DEBUG("playerbots", "LfgRoleCheckAction bot={} rolesDesired={}",
            bot->GetName(),
            uint32(setRoles.RolesDesired));

    return true;
}

bool LfgAcceptAction::IsUseful()
{
    if (!_botAI)
        return false;

    Player* bot = _botAI->GetBot();
    if (!bot || !bot->GetSession())
        return false;

    auto const& pending = _botAI->GetPendingLfgProposal();
    if (!pending || !pending->ProposalID)
        return false;

    return sLFGMgr->GetState(bot->GetGUID()) == lfg::LFG_STATE_PROPOSAL;
}

bool LfgAcceptAction::Execute(Event /*event*/)
{
    if (!_botAI)
        return false;

    Player* bot = _botAI->GetBot();
    if (!bot || !bot->GetSession())
        return false;

    auto const& pending = _botAI->GetPendingLfgProposal();
    if (!pending || !pending->ProposalID)
        return false;

    if (sLFGMgr->GetState(bot->GetGUID()) != lfg::LFG_STATE_PROPOSAL)
        return false;

    // AC noise gate: combat/dead → decline. No ResetStrategies / RandomBot Refresh.
    bool const accepted = !(bot->IsInCombat() || !bot->IsAlive());

    // Midnight: CMSG_DF_PROPOSAL_RESPONSE → HandleLfgProposalResultOpcode.
    // Do not paste AC WotLK CMSG_LFG_PROPOSAL_RESULT / QueuePacket.
    WorldPacket packet(CMSG_DF_PROPOSAL_RESPONSE);
    WorldPackets::LFG::DFProposalResponse response(std::move(packet));
    response.Ticket = pending->Ticket;
    response.InstanceID = pending->InstanceID;
    response.ProposalID = pending->ProposalID;
    response.Accepted = accepted;
    bot->GetSession()->HandleLfgProposalResultOpcode(response);

    _botAI->ClearPendingLfgProposal();

    if (Playerbots::GetLogLevel() >= 1)
        TC_LOG_DEBUG("playerbots", "LfgAcceptAction bot={} proposalId={} accepted={}",
            bot->GetName(),
            response.ProposalID,
            accepted ? "yes" : "no");

    return true;
}
