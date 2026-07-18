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
#include "Bot/Packet/BotBattlefieldStatusPackets.h"
#include "Bot/Packet/BotPacketParse.h"
#include "LFGPacketsCommon.h"
#include "Log.h"
#include "Opcodes.h"
#include "Player.h"
#include "PlayerbotsConfig.h"

namespace Playerbots::PacketHandler
{
namespace
{
void SoftDualHeader(Player const* bot, WorldPackets::Battleground::BattlefieldStatusHeader const& hdr, char const* opcodeLabel)
{
    if (!bot || Playerbots::GetLogLevel() < 1)
        return;

    ObjectGuid const botGuid = bot->GetGUID();
    if (!hdr.Ticket.RequesterGuid.IsEmpty() && hdr.Ticket.RequesterGuid != botGuid)
        TC_LOG_DEBUG("playerbots.packet",
            "BotPacketParse {} soft Ticket.RequesterGuid mismatch bot={} ticket={}",
            opcodeLabel, bot->GetName(), hdr.Ticket.RequesterGuid.ToString());

    if (hdr.Ticket.Type != WorldPackets::LFG::RideType::Battlegrounds &&
        hdr.Ticket.Type != WorldPackets::LFG::RideType::None)
        TC_LOG_DEBUG("playerbots.packet",
            "BotPacketParse {} soft Ticket.Type bot={} type={}",
            opcodeLabel, bot->GetName(), uint32(hdr.Ticket.Type));

    if (hdr.QueueID.empty())
        TC_LOG_DEBUG("playerbots.packet",
            "BotPacketParse {} soft empty QueueID bot={}",
            opcodeLabel, bot->GetName());
}
}

void HandleBattlefieldStatusNeedConfirmation(BotPlayerbotAI& ai, BotPlayerbotAI::QueuedSignal& signal)
{
    // Cleared unless Layer 2 OK (InBattlegroundQueue). Stash Ticket for port accept.
    signal.Name.clear();
    ai.ClearPendingBgStatus();

    Playerbots::PacketParse::BattlefieldStatusNeedConfirmationPayload parsed;
    if (!Playerbots::PacketParse::TryReadBattlefieldStatusNeedConfirmation(*signal.Packet, parsed))
        return;

    Player* bot = ai.GetBot();
    if (!bot || !bot->InBattlegroundQueue())
    {
        TC_LOG_ERROR("playerbots.packet",
            "BotPacketParse SMSG_BATTLEFIELD_STATUS_NEED_CONFIRMATION Layer-2 queue mismatch bot={} inQueue={} verifiedBuild={}",
            bot ? bot->GetName() : "?",
            bot && bot->InBattlegroundQueue() ? "yes" : "no",
            Playerbots::PacketParse::VERIFIED_BUILD);
        return;
    }

    SoftDualHeader(bot, parsed.Hdr, "SMSG_BATTLEFIELD_STATUS_NEED_CONFIRMATION");

    if (!parsed.Mapid && Playerbots::GetLogLevel() >= 1)
        TC_LOG_DEBUG("playerbots.packet",
            "BotPacketParse SMSG_BATTLEFIELD_STATUS_NEED_CONFIRMATION soft Mapid=0 bot={}",
            bot->GetName());

    if (Playerbots::GetLogLevel() >= 1)
        TC_LOG_DEBUG("playerbots.packet",
            "BotPacketParse SMSG_BATTLEFIELD_STATUS_NEED_CONFIRMATION ok bot={} map={} timeout={} role={} ticketId={}",
            bot->GetName(),
            parsed.Mapid,
            parsed.Timeout,
            uint32(parsed.Role),
            parsed.Hdr.Ticket.Id);

    BotPlayerbotAI::PendingBgStatus pending;
    pending.Kind = BotPlayerbotAI::BgStatusKind::NeedConfirmation;
    pending.Ticket = parsed.Hdr.Ticket;
    pending.Mapid = parsed.Mapid;
    pending.Timeout = parsed.Timeout;
    ai.SetPendingBgStatus(std::move(pending));
    signal.Name = "bg status";
}

void HandleBattlefieldStatusQueued(BotPlayerbotAI& ai, BotPlayerbotAI::QueuedSignal& signal)
{
    // Cleared unless Layer 2 OK (InBattlegroundQueue). Signal-only V1 (no leave-queue).
    signal.Name.clear();
    ai.ClearPendingBgStatus();

    Playerbots::PacketParse::BattlefieldStatusQueuedPayload parsed;
    if (!Playerbots::PacketParse::TryReadBattlefieldStatusQueued(*signal.Packet, parsed))
        return;

    Player* bot = ai.GetBot();
    if (!bot || !bot->InBattlegroundQueue())
    {
        TC_LOG_ERROR("playerbots.packet",
            "BotPacketParse SMSG_BATTLEFIELD_STATUS_QUEUED Layer-2 queue mismatch bot={} inQueue={} verifiedBuild={}",
            bot ? bot->GetName() : "?",
            bot && bot->InBattlegroundQueue() ? "yes" : "no",
            Playerbots::PacketParse::VERIFIED_BUILD);
        return;
    }

    SoftDualHeader(bot, parsed.Hdr, "SMSG_BATTLEFIELD_STATUS_QUEUED");

    if (Playerbots::GetLogLevel() >= 1)
        TC_LOG_DEBUG("playerbots.packet",
            "BotPacketParse SMSG_BATTLEFIELD_STATUS_QUEUED ok bot={} avgWait={} wait={} ticketId={}",
            bot->GetName(),
            parsed.AverageWaitTime,
            parsed.WaitTime,
            parsed.Hdr.Ticket.Id);

    BotPlayerbotAI::PendingBgStatus pending;
    pending.Kind = BotPlayerbotAI::BgStatusKind::Queued;
    pending.Ticket = parsed.Hdr.Ticket;
    ai.SetPendingBgStatus(std::move(pending));
    signal.Name = "bg status";
}

void HandleBattlefieldStatusActive(BotPlayerbotAI& ai, BotPlayerbotAI::QueuedSignal& signal)
{
    // Soft in-BG / still-queued dual. Signal-only V1 (no LeaveBG).
    signal.Name.clear();
    ai.ClearPendingBgStatus();

    Playerbots::PacketParse::BattlefieldStatusActivePayload parsed;
    if (!Playerbots::PacketParse::TryReadBattlefieldStatusActive(*signal.Packet, parsed))
        return;

    Player* bot = ai.GetBot();
    if (!bot)
        return;

    bool const inBg = bot->InBattleground();
    bool const inQueue = bot->InBattlegroundQueue();
    if (!inBg && !inQueue)
    {
        TC_LOG_ERROR("playerbots.packet",
            "BotPacketParse SMSG_BATTLEFIELD_STATUS_ACTIVE Layer-2 not in BG/queue bot={} verifiedBuild={}",
            bot->GetName(),
            Playerbots::PacketParse::VERIFIED_BUILD);
        return;
    }

    SoftDualHeader(bot, parsed.Hdr, "SMSG_BATTLEFIELD_STATUS_ACTIVE");

    if (!inBg && Playerbots::GetLogLevel() >= 1)
        TC_LOG_DEBUG("playerbots.packet",
            "BotPacketParse SMSG_BATTLEFIELD_STATUS_ACTIVE soft not yet InBattleground bot={} inQueue={}",
            bot->GetName(),
            inQueue ? "yes" : "no");

    if (parsed.Mapid && bot->GetMapId() != parsed.Mapid && inBg && Playerbots::GetLogLevel() >= 1)
        TC_LOG_DEBUG("playerbots.packet",
            "BotPacketParse SMSG_BATTLEFIELD_STATUS_ACTIVE soft Mapid dual bot={} live={} payload={}",
            bot->GetName(),
            bot->GetMapId(),
            parsed.Mapid);

    if (Playerbots::GetLogLevel() >= 1)
        TC_LOG_DEBUG("playerbots.packet",
            "BotPacketParse SMSG_BATTLEFIELD_STATUS_ACTIVE ok bot={} map={} shutdown={} start={} ticketId={}",
            bot->GetName(),
            parsed.Mapid,
            parsed.ShutdownTimer,
            parsed.StartTimer,
            parsed.Hdr.Ticket.Id);

    BotPlayerbotAI::PendingBgStatus pending;
    pending.Kind = BotPlayerbotAI::BgStatusKind::Active;
    pending.Ticket = parsed.Hdr.Ticket;
    pending.Mapid = parsed.Mapid;
    ai.SetPendingBgStatus(std::move(pending));
    signal.Name = "bg status";
}

} // namespace Playerbots::PacketHandler
