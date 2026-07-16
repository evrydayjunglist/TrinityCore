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

#include "BotPlayerbotAI.h"
#include "AiFactory.h"
#include "Bot/Packet/BotGuildInvitePacket.h"
#include "Bot/Packet/BotPartyInvitePacket.h"
#include "Bot/Packet/BotPacketParse.h"
#include "Bot/Packet/BotResurrectRequestPacket.h"
#include "Engine.h"
#include "Group.h"
#include "Log.h"
#include "Opcodes.h"
#include "Player.h"
#include "PlayerbotsConfig.h"
#include "Random.h"
#include "RandomPlayerbotMgr.h"
#include "WorldPacket.h"
#include <algorithm>
#include <unordered_map>

namespace
{
// AC's botOutgoingPacketHandlers shape (mod-playerbots-master PlayerbotAI.cpp:178-223).
// Signal name always; payload parse is opt-in per opcode behind the three-layer gate
// (playerbots-bot-packet-payload-parse-handoff.md).
struct PacketSignalEntry
{
    char const* SignalName;
    bool CopiesPayload; // when PayloadParse.Enable, enqueue a WorldPacket copy for tick-time parse
};

PacketSignalEntry const* LookupPacketSignal(uint32 opcode)
{
    static std::unordered_map<uint32, PacketSignalEntry> const registry = {
        { SMSG_PARTY_INVITE, { "group invite signal", true } },
        { SMSG_GUILD_INVITE, { "guild invite signal", true } },
        { SMSG_RESURRECT_REQUEST, { "resurrect request signal", true } },
    };

    auto itr = registry.find(opcode);
    return itr != registry.end() ? &itr->second : nullptr;
}

constexpr size_t BOT_SIGNAL_QUEUE_MAX = 32;
constexpr size_t BOT_SIGNAL_PENDING_MAX = 32;
constexpr uint8 BOT_SIGNAL_TTL_TICKS = 3;
}

BotPlayerbotAI::BotPlayerbotAI(Player* bot) : PlayerbotAIBase(bot)
{
    _context = AiFactory::CreateContext(this, bot);
    _engine = std::make_unique<Engine>(this, _context.get());
    ResetStrategies();
}

void BotPlayerbotAI::ResetStrategies()
{
    if (!_engine)
        return;

    _engine->RemoveAllStrategies();

    std::string appliedStrategies;
    if (HasMaster())
    {
        _engine->AddStrategy("follow");
        _engine->AddStrategy("attack");
        appliedStrategies = "follow,attack";
    }
    else if (Player* bot = GetBot(); bot && sRandomPlayerbotMgr->IsRandomBot(bot->GetGUID()) &&
        roll_chance(Playerbots::GetRandomBotRpgChance()))
    {
        _engine->AddStrategy("newrpg");
        appliedStrategies = "newrpg";
        _rpgInfo.Reset();
    }
    else
    {
        _engine->AddStrategy("passive");
        appliedStrategies = "passive";
    }

    _engine->Init();

    if (Playerbots::GetLogLevel() >= 1)
    {
        TC_LOG_DEBUG("playerbots", "BotPlayerbotAI::ResetStrategies bot={} master={} strategies={}",
            GetBot() ? GetBot()->GetName() : "?",
            HasMaster() ? "yes" : "no",
            appliedStrategies);
    }
}

void BotPlayerbotAI::HandleBotOutgoingPacket(WorldPacket const& packet)
{
    // Sender-thread cheap path: opcode registry + optional packet copy enqueue only.
    PacketSignalEntry const* entry = LookupPacketSignal(packet.GetOpcode());
    if (!entry)
        return;

    QueuedSignal queued;
    queued.Name = entry->SignalName;
    if (entry->CopiesPayload && Playerbots::GetPacketPayloadParseEnabled())
        queued.Packet = packet; // WorldPacket copy

    std::lock_guard<std::mutex> lock(_signalMutex);
    if (_signalQueue.size() >= BOT_SIGNAL_QUEUE_MAX)
    {
        _signalQueue.erase(_signalQueue.begin());
        if (Playerbots::GetLogLevel() >= 1)
            TC_LOG_DEBUG("playerbots", "BotPlayerbotAI signal queue overflow (dropped oldest) for bot {}",
                GetBot() ? GetBot()->GetName() : "?");
    }
    _signalQueue.push_back(std::move(queued));
    if (Playerbots::GetLogLevel() >= 1)
        TC_LOG_DEBUG("playerbots", "BotPlayerbotAI queued signal '{}' for bot {}",
            entry->SignalName, GetBot() ? GetBot()->GetName() : "?");
}

bool BotPlayerbotAI::ConsumeSignal(std::string const& name)
{
    auto itr = std::ranges::find_if(_pendingSignals, [&name](PendingSignal const& signal)
    {
        return signal.Name == name;
    });

    if (itr == _pendingSignals.end())
        return false;

    _pendingSignals.erase(itr);
    return true;
}

void BotPlayerbotAI::ProcessPayloadOnTick(QueuedSignal& signal)
{
    if (!signal.Packet)
        return;

    switch (signal.Packet->GetOpcode())
    {
        case SMSG_PARTY_INVITE:
        {
            Playerbots::PacketParse::PartyInvitePayload parsed;
            if (!Playerbots::PacketParse::TryReadPartyInvite(*signal.Packet, parsed))
            {
                // Layer 1 failed — ERROR already logged; keep signal for poll/signal fallback.
                return;
            }

            // Layer 2: cross-check InviterGUID vs live group invite leader.
            Player* bot = GetBot();
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
            break;
        }
        case SMSG_GUILD_INVITE:
        {
            Playerbots::PacketParse::GuildInvitePayload parsed;
            if (!Playerbots::PacketParse::TryReadGuildInvite(*signal.Packet, parsed))
                return;

            // Layer 2: pending guild id vs parsed GuildGUID counter (GetGuildIdInvited dual).
            Player* bot = GetBot();
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
            break;
        }
        case SMSG_RESURRECT_REQUEST:
        {
            Playerbots::PacketParse::ResurrectRequestPayload parsed;
            if (!Playerbots::PacketParse::TryReadResurrectRequest(*signal.Packet, parsed))
                return;

            // Layer 2: pending resurrection dual vs parsed offerer GUID.
            Player* bot = GetBot();
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
            break;
        }
        default:
            break;
    }
}

void BotPlayerbotAI::UpdateAIInternal(uint32 diff)
{
    std::vector<QueuedSignal> drained;
    {
        std::lock_guard<std::mutex> lock(_signalMutex);
        drained.swap(_signalQueue);
    }

    for (QueuedSignal& queued : drained)
    {
        ProcessPayloadOnTick(queued);

        if (_pendingSignals.size() >= BOT_SIGNAL_PENDING_MAX)
        {
            if (Playerbots::GetLogLevel() >= 1)
                TC_LOG_DEBUG("playerbots", "BotPlayerbotAI pending signal overflow (dropped oldest) for bot {}",
                    GetBot() ? GetBot()->GetName() : "?");

            _pendingSignals.erase(_pendingSignals.begin());
        }

        _pendingSignals.push_back({ std::move(queued.Name), BOT_SIGNAL_TTL_TICKS });
    }

    if (_engine)
        _engine->DoNextAction(diff);

    for (auto itr = _pendingSignals.begin(); itr != _pendingSignals.end();)
    {
        if (itr->TicksRemaining > 1)
        {
            --itr->TicksRemaining;
            ++itr;
            continue;
        }

        if (Playerbots::GetLogLevel() >= 1)
            TC_LOG_DEBUG("playerbots", "BotPlayerbotAI expired signal '{}' for bot {}",
                itr->Name, GetBot() ? GetBot()->GetName() : "?");

        itr = _pendingSignals.erase(itr);
    }
}
