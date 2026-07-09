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
#include "Engine.h"
#include "Log.h"
#include "Opcodes.h"
#include "Player.h"
#include "PlayerbotsConfig.h"
#include "Random.h"
#include "RandomPlayerbotMgr.h"
#include "WorldPacket.h"
#include <unordered_map>

namespace
{
// AC's botOutgoingPacketHandlers shape (mod-playerbots-master PlayerbotAI.cpp:178-223): a static
// opcode -> named-trigger table. AC parses several of these packets inline; this fork deliberately
// does NOT (handoff § 0 — Midnight wire formats churn, so payload parsing would be a standing merge
// liability). Here the opcode is purely a wake-up signal: the mapped trigger fires on the bot's
// tick and its action reads live server state via public core APIs. V1 registers exactly one entry;
// the table grows only as future SMSG-reactive features land (each a one-line addition + wiring).
std::string const* LookupPacketSignal(uint32 opcode)
{
    static std::unordered_map<uint32, std::string> const registry = {
        { SMSG_PARTY_INVITE, "group invite signal" },
    };

    auto itr = registry.find(opcode);
    return itr != registry.end() ? &itr->second : nullptr;
}

// Bounded per-bot signal queue. A lost signal (overflow / logout race) self-heals: every
// signal-driven feature must stay correct if the signal never arrives — the underlying state is
// still poll-readable (handoff § 7). Small: signals are drained every tick, so this only ever
// fills if a bot is somehow not ticking.
constexpr size_t BOT_SIGNAL_QUEUE_MAX = 32;
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
        // AC: masterless random bots run the "newrpg" wander/grind/quest-giver loop instead of
        // sitting passive; RandomBotRpgChance is this fork's single on/off-with-a-dial knob for it.
        _engine->AddStrategy("newrpg");
        appliedStrategies = "newrpg";
        _rpgInfo.Reset(); // Gate 10b: fresh state machine whenever the RPG strategy is (re)applied
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
    // Runs on arbitrary map/World sender threads — keep this cheap and touch nothing but the
    // guarded queue. Opcode-as-signal only: read the opcode, never the payload (handoff § 0).
    std::string const* signalName = LookupPacketSignal(packet.GetOpcode());
    if (!signalName)
        return; // the common case: no lock, no allocation

    std::lock_guard<std::mutex> lock(_signalMutex);
    if (_signalQueue.size() >= BOT_SIGNAL_QUEUE_MAX)
    {
        // Drop-oldest so a runaway producer can't grow unbounded; the poll fallback still catches
        // whatever was dropped.
        _signalQueue.erase(_signalQueue.begin());
        if (Playerbots::GetLogLevel() >= 1)
            TC_LOG_DEBUG("playerbots", "BotPlayerbotAI signal queue overflow (dropped oldest) for bot {}",
                GetBot() ? GetBot()->GetName() : "?");
    }
    _signalQueue.push_back(*signalName);
}

bool BotPlayerbotAI::ConsumeSignal(std::string const& name)
{
    auto itr = _firedSignals.find(name);
    if (itr == _firedSignals.end())
        return false;

    _firedSignals.erase(itr);
    return true;
}

void BotPlayerbotAI::UpdateAIInternal(uint32 diff)
{
    // Drain the packet-observation signals captured on sender threads since the last tick. Swap the
    // queue out under the lock (tiny critical section), then rebuild the per-tick fired set outside
    // it. From here on everything is single-threaded on the bot's own tick (handoff § 5.4). Signals
    // live for exactly one tick — a consume-on-read SignalTrigger claims theirs during DoNextAction.
    std::vector<std::string> drained;
    {
        std::lock_guard<std::mutex> lock(_signalMutex);
        drained.swap(_signalQueue);
    }

    _firedSignals.clear();
    for (std::string& name : drained)
        _firedSignals.insert(std::move(name));

    if (_engine)
        _engine->DoNextAction(diff);
}
