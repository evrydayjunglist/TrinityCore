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
#include "Ai/Class/Rogue/RogueSpellIds.h"
#include "AiFactory.h"
#include "Bot/PacketHandler/BotPacketSignal.h"
#include "Engine.h"
#include "Log.h"
#include "Player.h"
#include "PlayerbotsConfig.h"
#include "Random.h"
#include "RandomPlayerbotMgr.h"
#include "WorldPacket.h"
#include <algorithm>

namespace
{
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

    Player* bot = GetBot();
    bool const assassinationPilot = Playerbots::Rogue::Assassination::IsAssassinationRogue(bot);

    std::string appliedStrategies;
    if (HasMaster())
    {
        _engine->AddStrategy("follow");
        _engine->AddStrategy("attack");
        _engine->AddStrategy("flee");
        appliedStrategies = "follow,attack,flee";
        // Gate 14 — attach pilot class strategies from class + GetPrimarySpecialization.
        if (assassinationPilot)
        {
            _engine->AddStrategy("assassination");
            _engine->AddStrategy("rogue buff");
            appliedStrategies += ",assassination,rogue buff";
        }
    }
    else if (bot && sRandomPlayerbotMgr->IsRandomBot(bot->GetGUID()) &&
        roll_chance(Playerbots::GetRandomBotRpgChance()))
    {
        _engine->AddStrategy("newrpg");
        _engine->AddStrategy("flee");
        appliedStrategies = "newrpg,flee";
        if (assassinationPilot)
        {
            _engine->AddStrategy("assassination");
            _engine->AddStrategy("rogue buff");
            appliedStrategies += ",assassination,rogue buff";
        }
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
    Playerbots::PacketHandler::PacketSignalEntry const* entry =
        Playerbots::PacketHandler::LookupPacketSignal(packet.GetOpcode());
    if (!entry)
        return;

    QueuedSignal queued;
    queued.Opcode = packet.GetOpcode();
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

bool BotPlayerbotAI::TellMaster(std::string_view text)
{
    Player* bot = GetBot();
    Player* master = GetMaster();
    if (!bot || !master)
    {
        if (Playerbots::GetLogLevel() >= 1)
            TC_LOG_DEBUG("playerbots", "TellMaster skipped (no master) bot={} text='{}'",
                bot ? bot->GetName() : "?",
                text);
        return false;
    }

    bot->Whisper(text, LANG_UNIVERSAL, master);
    return true;
}

void BotPlayerbotAI::ProcessPayloadOnTick(QueuedSignal& signal)
{
    Playerbots::PacketHandler::PacketSignalEntry const* entry =
        Playerbots::PacketHandler::LookupPacketSignal(signal.Opcode);
    if (!entry)
        return;

    if (!signal.Packet)
    {
        // Parse-gated reactions with no poll dual: do not fire when PayloadParse is off.
        if (entry->ClearSignalOnNoPayload)
        {
            if (entry->ClearPending)
                (this->*entry->ClearPending)();
            signal.Name.clear();
        }
        return;
    }

    if (entry->Handler)
        entry->Handler(*this, signal);
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

        // Buy-failed clears Name on non-V1 / Layer fail; placeholder "buy failed" must never
        // reach pending (PayloadParse off, or rewrite skipped).
        if (queued.Name.empty() || queued.Name == "buy failed")
            continue;

        if (_pendingSignals.size() >= BOT_SIGNAL_PENDING_MAX)
        {
            if (Playerbots::GetLogLevel() >= 1)
                TC_LOG_DEBUG("playerbots", "BotPlayerbotAI pending signal overflow (dropped oldest) for bot {}",
                    GetBot() ? GetBot()->GetName() : "?");

            _pendingSignals.erase(_pendingSignals.begin());
        }

        _pendingSignals.push_back({ std::move(queued.Name), BOT_SIGNAL_TTL_TICKS });
    }

    // Pending-signal TTL must only burn on ticks where ProcessTriggers ran. Default GCD (500ms)
    // is longer than TTL (3 × ReactDelay ~100ms): signal-only reactions (e.g. "group set leader")
    // otherwise expire while follow keeps the engine on cooldown — poll duals masked that bug.
    bool triggersProcessed = false;
    if (_engine)
        _engine->DoNextAction(diff, triggersProcessed);

    if (!triggersProcessed)
        return;

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
