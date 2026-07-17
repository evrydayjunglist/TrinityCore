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

#include "PetitionActions.h"
#include "BotPlayerbotAI.h"
#include "Log.h"
#include "Opcodes.h"
#include "PetitionMgr.h"
#include "PetitionPackets.h"
#include "Player.h"
#include "PlayerbotsConfig.h"
#include "WorldPacket.h"
#include "WorldSession.h"

namespace
{
// Layer-2 stash required. Master-alt: owner must be master (same-account client UI cannot
// offer). Passive / GM `.playerbot login`: no master — sign any offered petition (offer already
// targeted this bot via SMSG_PETITION_SHOW_SIGNATURES).
bool ResolvePetitionOffer(BotPlayerbotAI* botAI, Player*& bot, Player*& master, ObjectGuid& petitionGuid)
{
    if (!botAI)
        return false;

    bot = botAI->GetBot();
    master = botAI->GetMaster();
    if (!bot || !bot->GetSession())
        return false;

    if (bot->GetGuildId() || bot->GetGuildIdInvited())
        return false;

    petitionGuid = botAI->GetPendingPetitionOffer();
    if (petitionGuid.IsEmpty())
        return false;

    Petition const* petition = sPetitionMgr->GetPetition(petitionGuid);
    if (!petition || petition->OwnerGuid == bot->GetGUID())
        return false;

    if (master && petition->OwnerGuid != master->GetGUID())
        return false;

    // TC same-account rule: one signer per account per petition — don't keep retrying.
    if (petition->IsPetitionSignedByAccount(bot->GetSession()->GetAccountId()))
        return false;

    return true;
}
}

bool PetitionSignAction::IsUseful()
{
    Player* bot = nullptr;
    Player* master = nullptr;
    ObjectGuid petitionGuid;
    return ResolvePetitionOffer(_botAI, bot, master, petitionGuid);
}

bool PetitionSignAction::Execute(Event /*event*/)
{
    Player* bot = nullptr;
    Player* master = nullptr;
    ObjectGuid petitionGuid;
    if (!ResolvePetitionOffer(_botAI, bot, master, petitionGuid))
        return false;

    // Midnight: CMSG_SIGN_PETITION → SignPetition (PetitionGUID + Choice); Choice = 0 accept.
    // Do not paste AC WotLK CMSG_PETITION_SIGN / MSG_PETITION_DECLINE bytes.
    WorldPacket packet(CMSG_SIGN_PETITION);
    WorldPackets::Petition::SignPetition sign(std::move(packet));
    sign.PetitionGUID = petitionGuid;
    sign.Choice = 0;
    bot->GetSession()->HandleSignPetition(sign);

    _botAI->ClearPendingPetitionOffer();

    Petition const* petition = sPetitionMgr->GetPetition(petitionGuid);
    bool const signedOk = petition && petition->IsPetitionSignedByAccount(bot->GetSession()->GetAccountId());

    if (Playerbots::GetLogLevel() >= 1)
        TC_LOG_DEBUG("playerbots", "PetitionSignAction bot={} offerer={} petition={} signed={}",
            bot->GetName(),
            master ? master->GetName() : (petition ? petition->OwnerGuid.ToString() : "?"),
            petitionGuid.ToString(),
            signedOk ? "yes" : "no");

    return signedOk;
}
