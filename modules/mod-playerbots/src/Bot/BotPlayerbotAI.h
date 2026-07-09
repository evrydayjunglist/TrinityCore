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

#ifndef TRINITY_BOT_PLAYERBOT_AI_H
#define TRINITY_BOT_PLAYERBOT_AI_H

#include "AiObjectContext.h"
#include "Bot/Rpg/NewRpgInfo.h"
#include "Engine.h"
#include "PlayerbotAIBase.h"
#include <memory>
#include <mutex>
#include <string>
#include <unordered_set>
#include <vector>

class AiObjectContext;
class Engine;
class WorldPacket;

// Gate 6: bot AI with AC-shaped Engine + passive strategy.
class BotPlayerbotAI : public PlayerbotAIBase
{
public:
    explicit BotPlayerbotAI(Player* bot);
    ~BotPlayerbotAI() = default;

    void ResetStrategies();
    Engine* GetEngine() const { return _engine.get(); }
    AiObjectContext* GetAiObjectContext() const { return _context.get(); }

    Player* GetMaster() const { return _master; }
    void SetMaster(Player* master) { _master = master; }
    bool HasMaster() const { return _master != nullptr; }

    // Gate 10b — per-bot RPG state machine + lifecycle counters + abandon set (AC keeps the
    // same trio as public members rpgInfo/rpgStatistic/lowPriorityQuest on PlayerbotAI).
    NewRpgInfo& GetRpgInfo() { return _rpgInfo; }
    NewRpgStatistic& GetRpgStatistics() { return _rpgStatistic; }
    std::unordered_set<uint32>& GetLowPriorityQuests() { return _lowPriorityQuest; }

    // Quests proven impossible to complete for ANY player (a COMPLETE quest with no ender
    // anywhere in world data, e.g. the auto-granted junk 55660 "Time Trials"). Distinct from
    // the low-priority set (that = "tried a POI, stalled"; this = "provably unactionable").
    // In-memory only (owner directive: never DropQuest/AbandonQuest/DB-touch); per-session, so
    // it starts empty each login and a later data fix or new objective handler re-includes the
    // quest automatically. See playerbots-rpg-active-questgiver-seeking-handoff.md §4.
    std::unordered_set<uint32>& GetUnactionableQuests() { return _unactionableQuest; }

    // Packet-observation signal layer (playerbots-bot-packet-observation-handoff.md § 5). AC's
    // PlayerbotAI::HandleBotOutgoingPacket analog. Called from the module's ServerScript observer
    // on ARBITRARY map/World sender threads (WorldSession::SendPacket runs there) — so it does
    // nothing but a static opcode->trigger-name registry test and, on a hit, a cheap bounded
    // enqueue under _signalMutex. Opcode-as-signal ONLY: the packet payload is never read (§ 0).
    // All reaction happens single-threaded on the bot's own tick (the UpdateAIInternal drain).
    void HandleBotOutgoingPacket(WorldPacket const& packet);

    // Consume-on-read: tick-thread only (SignalTrigger::IsActive during Engine::DoNextAction). The
    // first trigger to test its signal name this tick claims it; returns false once consumed.
    bool ConsumeSignal(std::string const& name);

protected:
    void UpdateAIInternal(uint32 diff) override;

private:
    std::unique_ptr<AiObjectContext> _context;
    std::unique_ptr<Engine> _engine;
    Player* _master = nullptr;
    NewRpgInfo _rpgInfo;
    NewRpgStatistic _rpgStatistic;
    std::unordered_set<uint32> _lowPriorityQuest;
    std::unordered_set<uint32> _unactionableQuest;

    // Cross-thread signal handoff. _signalQueue is written by HandleBotOutgoingPacket on sender
    // threads and drained (swapped out) at the top of each tick under _signalMutex; _firedSignals
    // is the per-tick fired set, only ever touched on the bot's own tick thread (drain + consume),
    // so it needs no lock of its own.
    std::mutex _signalMutex;
    std::vector<std::string> _signalQueue;
    std::unordered_set<std::string> _firedSignals;
};

#endif
