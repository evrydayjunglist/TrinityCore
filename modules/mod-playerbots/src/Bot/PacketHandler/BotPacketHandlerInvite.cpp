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

#include "Bot/PacketHandler/BotPacketSignal.h"
#include "Bot/Packet/BotPacketParse.h"
#include "Log.h"
#include "Opcodes.h"
#include "PlayerbotsConfig.h"
#include "Bot/Packet/BotPartyInvitePacket.h"
#include "Bot/Packet/BotGuildInvitePacket.h"
#include "Bot/Packet/BotResurrectRequestPacket.h"
#include "Bot/Packet/BotPetitionShowSignaturesPacket.h"
#include "Bot/Packet/BotDuelRequestedPacket.h"
#include "Group.h"
#include "PetitionMgr.h"
#include "Player.h"
#include "ObjectAccessor.h"
#include "Map.h"

namespace Playerbots::PacketHandler
{
void HandlePartyInvite(BotPlayerbotAI& ai, BotPlayerbotAI::QueuedSignal& signal)
{

        Playerbots::PacketParse::PartyInvitePayload parsed;
        if (!Playerbots::PacketParse::TryReadPartyInvite(*signal.Packet, parsed))
        {
            // Layer 1 failed — ERROR already logged; keep signal for poll/signal fallback.
            return;
        }

        // Layer 2: cross-check InviterGUID vs live group invite leader.
        Player* bot = ai.GetBot();
        Group* invite = bot ? bot->GetGroupInvite() : nullptr;
        ObjectGuid const liveLeader = invite ? invite->GetLeaderGUID() : ObjectGuid::Empty;
        if (liveLeader.IsEmpty() || liveLeader != parsed.InviterGUID)
        {
            TC_LOG_ERROR("playerbots.packet",
                "BotPacketParse SMSG_PARTY_INVITE Layer-2 mismatch bot={} parsedInviter={} liveLeader={} verifiedBuild={}",
                bot ? bot->GetName() : "?",
                parsed.InviterGUID.ToString(),
                liveLeader.ToString(),
                Playerbots::PacketParse::VERIFIED_BUILD);
            // Prefer live state for the accept action; signal still fires.
            return;
        }

        if (Playerbots::GetLogLevel() >= 1)
            TC_LOG_DEBUG("playerbots.packet",
                "BotPacketParse SMSG_PARTY_INVITE ok bot={} inviter={} name={}",
                bot ? bot->GetName() : "?",
                parsed.InviterGUID.ToString(),
                parsed.InviterName);
}

void HandleGuildInvite(BotPlayerbotAI& ai, BotPlayerbotAI::QueuedSignal& signal)
{

        Playerbots::PacketParse::GuildInvitePayload parsed;
        if (!Playerbots::PacketParse::TryReadGuildInvite(*signal.Packet, parsed))
            return;

        // Layer 2: pending guild id vs parsed GuildGUID counter (GetGuildIdInvited dual).
        Player* bot = ai.GetBot();
        ObjectGuid::LowType const liveInvited = bot ? bot->GetGuildIdInvited() : 0;
        ObjectGuid::LowType const parsedGuildId = parsed.GuildGUID.GetCounter();
        if (!liveInvited || liveInvited != parsedGuildId)
        {
            TC_LOG_ERROR("playerbots.packet",
                "BotPacketParse SMSG_GUILD_INVITE Layer-2 mismatch bot={} parsedGuild={} liveInvited={} verifiedBuild={}",
                bot ? bot->GetName() : "?",
                parsed.GuildGUID.ToString(),
                liveInvited,
                Playerbots::PacketParse::VERIFIED_BUILD);
            return;
        }

        if (Playerbots::GetLogLevel() >= 1)
            TC_LOG_DEBUG("playerbots.packet",
                "BotPacketParse SMSG_GUILD_INVITE ok bot={} guild={} name={} inviter={}",
                bot ? bot->GetName() : "?",
                parsed.GuildGUID.ToString(),
                parsed.GuildName,
                parsed.InviterName);
}

void HandleResurrectRequest(BotPlayerbotAI& ai, BotPlayerbotAI::QueuedSignal& signal)
{

        Playerbots::PacketParse::ResurrectRequestPayload parsed;
        if (!Playerbots::PacketParse::TryReadResurrectRequest(*signal.Packet, parsed))
            return;

        // Layer 2: pending resurrection dual vs parsed offerer GUID.
        Player* bot = ai.GetBot();
        if (!bot || !bot->IsResurrectRequested() ||
            !bot->IsResurrectRequestedBy(parsed.ResurrectOffererGUID))
        {
            TC_LOG_ERROR("playerbots.packet",
                "BotPacketParse SMSG_RESURRECT_REQUEST Layer-2 mismatch bot={} parsedOfferer={} liveRequested={} verifiedBuild={}",
                bot ? bot->GetName() : "?",
                parsed.ResurrectOffererGUID.ToString(),
                bot && bot->IsResurrectRequested() ? bot->GetResurrectRequesterGUID().ToString() : "none",
                Playerbots::PacketParse::VERIFIED_BUILD);
            return;
        }

        if (Playerbots::GetLogLevel() >= 1)
            TC_LOG_DEBUG("playerbots.packet",
                "BotPacketParse SMSG_RESURRECT_REQUEST ok bot={} offerer={} name={} spell={}",
                bot->GetName(),
                parsed.ResurrectOffererGUID.ToString(),
                parsed.Name,
                parsed.SpellID);
}

void HandlePetitionShowSignatures(BotPlayerbotAI& ai, BotPlayerbotAI::QueuedSignal& signal)
{

        Playerbots::PacketParse::PetitionShowSignaturesPayload parsed;
        if (!Playerbots::PacketParse::TryReadPetitionShowSignatures(*signal.Packet, parsed))
            return;

        // Layer 2: PetitionMgr item/owner dual (+ PetitionID == item counter).
        Petition const* petition = sPetitionMgr->GetPetition(parsed.Item);
        bool const layer2Ok = petition
            && petition->OwnerGuid == parsed.Owner
            && int32(parsed.Item.GetCounter()) == parsed.PetitionID;
        if (!layer2Ok)
        {
            TC_LOG_ERROR("playerbots.packet",
                "BotPacketParse SMSG_PETITION_SHOW_SIGNATURES Layer-2 mismatch bot={} parsedItem={} parsedOwner={} petitionId={} liveOwner={} verifiedBuild={}",
                ai.GetBot() ? ai.GetBot()->GetName() : "?",
                parsed.Item.ToString(),
                parsed.Owner.ToString(),
                parsed.PetitionID,
                petition ? petition->OwnerGuid.ToString() : "none",
                Playerbots::PacketParse::VERIFIED_BUILD);
            return;
        }

        ai.SetPendingPetitionOffer(parsed.Item);

        if (Playerbots::GetLogLevel() >= 1)
            TC_LOG_DEBUG("playerbots.packet",
                "BotPacketParse SMSG_PETITION_SHOW_SIGNATURES ok bot={} item={} owner={} petitionId={} signatures={}",
                ai.GetBot() ? ai.GetBot()->GetName() : "?",
                parsed.Item.ToString(),
                parsed.Owner.ToString(),
                parsed.PetitionID,
                parsed.Signatures.size());
}

void HandleDuelRequested(BotPlayerbotAI& ai, BotPlayerbotAI::QueuedSignal& signal)
{

        // Cleared unless Layer 2 OK + master challenger (V1). Enable=0: poll twin may still accept.
        signal.Name.clear();
        ai.ClearPendingDuelArbiter();

        Playerbots::PacketParse::DuelRequestedPayload parsed;
        if (!Playerbots::PacketParse::TryReadDuelRequested(*signal.Packet, parsed))
            return;

        Player* bot = ai.GetBot();
        if (!bot || !bot->duel || bot->duel->State != DUEL_STATE_CHALLENGED)
        {
            TC_LOG_ERROR("playerbots.packet",
                "BotPacketParse SMSG_DUEL_REQUESTED Layer-2 mismatch bot={} duelState={} verifiedBuild={}",
                bot ? bot->GetName() : "?",
                bot && bot->duel ? uint32(bot->duel->State) : 0u,
                Playerbots::PacketParse::VERIFIED_BUILD);
            return;
        }

        ObjectGuid const liveArbiter = *bot->m_playerData->DuelArbiter;
        if (liveArbiter.IsEmpty() || liveArbiter != parsed.ArbiterGUID)
        {
            TC_LOG_ERROR("playerbots.packet",
                "BotPacketParse SMSG_DUEL_REQUESTED Layer-2 Arbiter mismatch bot={} parsed={} live={} verifiedBuild={}",
                bot->GetName(),
                parsed.ArbiterGUID.ToString(),
                liveArbiter.ToString(),
                Playerbots::PacketParse::VERIFIED_BUILD);
            return;
        }

        if (!bot->duel->Initiator)
        {
            TC_LOG_ERROR("playerbots.packet",
                "BotPacketParse SMSG_DUEL_REQUESTED Layer-2 null Initiator bot={} verifiedBuild={}",
                bot->GetName(),
                Playerbots::PacketParse::VERIFIED_BUILD);
            return;
        }

        ObjectGuid const liveInitiator = bot->duel->Initiator->GetGUID();
        if (liveInitiator != parsed.RequestedByGUID)
        {
            TC_LOG_ERROR("playerbots.packet",
                "BotPacketParse SMSG_DUEL_REQUESTED Layer-2 Initiator mismatch bot={} parsed={} live={} verifiedBuild={}",
                bot->GetName(),
                parsed.RequestedByGUID.ToString(),
                liveInitiator.ToString(),
                Playerbots::PacketParse::VERIFIED_BUILD);
            return;
        }

        // Soft: bot is the challenger observing its own SMSG — clear without Layer-2 ERROR.
        if (bot->duel->Initiator == bot)
        {
            if (Playerbots::GetLogLevel() >= 1)
                TC_LOG_DEBUG("playerbots.packet",
                    "BotPacketParse SMSG_DUEL_REQUESTED soft initiator self-skip bot={}",
                    bot->GetName());
            return;
        }

        // Soft: RequestedBy should be a player; map-local when practical (no cold remote query).
        if (!parsed.RequestedByGUID.IsPlayer())
        {
            if (Playerbots::GetLogLevel() >= 1)
                TC_LOG_DEBUG("playerbots.packet",
                    "BotPacketParse SMSG_DUEL_REQUESTED soft non-player RequestedBy bot={} requestedBy={}",
                    bot->GetName(),
                    parsed.RequestedByGUID.ToString());
        }
        else if (Player* challenger = ObjectAccessor::GetPlayer(*bot, parsed.RequestedByGUID); !challenger)
        {
            if (Playerbots::GetLogLevel() >= 1)
                TC_LOG_DEBUG("playerbots.packet",
                    "BotPacketParse SMSG_DUEL_REQUESTED soft RequestedBy not map-local bot={} requestedBy={}",
                    bot->GetName(),
                    parsed.RequestedByGUID.ToString());
        }

        // Soft: ToTheDeath unconstrained (false expected from current SpellEffects sender).
        if (parsed.ToTheDeath && Playerbots::GetLogLevel() >= 1)
            TC_LOG_DEBUG("playerbots.packet",
                "BotPacketParse SMSG_DUEL_REQUESTED soft ToTheDeath=true bot={}",
                bot->GetName());

        Player* master = ai.GetMaster();
        bool const masterChallenger = master && parsed.RequestedByGUID == master->GetGUID();

        if (Playerbots::GetLogLevel() >= 1)
            TC_LOG_DEBUG("playerbots.packet",
                "BotPacketParse SMSG_DUEL_REQUESTED ok bot={} arbiter={} requestedBy={} toTheDeath={} masterChallenger={}",
                bot->GetName(),
                parsed.ArbiterGUID.ToString(),
                parsed.RequestedByGUID.ToString(),
                parsed.ToTheDeath ? "yes" : "no",
                masterChallenger ? "yes" : "no");

        // V1 FollowMaster: only enqueue accept signal for master challenger.
        if (!masterChallenger)
            return;

        ai.SetPendingDuelArbiter(parsed.ArbiterGUID);
        signal.Name = "duel requested";
}

} // namespace Playerbots::PacketHandler
