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

#ifndef TRINITY_PLAYERBOT_SIGNAL_TRIGGER_H
#define TRINITY_PLAYERBOT_SIGNAL_TRIGGER_H

#include "Trigger.h"
#include <string>
#include <utility>

class BotPlayerbotAI;

// Consume-on-read externally-fired trigger for the packet-observation signal layer
// (playerbots-bot-packet-observation-handoff.md § 5.5). Its name doubles as the signal name the
// opcode registry enqueues (LookupPacketSignal in BotPlayerbotAI.cpp): it is active exactly once
// per delivered signal, IsActive() consuming it from the bot's per-tick fired set. This is the
// fork's TC-native analog of AC's ExternalEventHelper-driven packet triggers — it plugs into the
// existing poll-based Engine::ProcessTriggers with no engine changes. The triggered ACTION reads
// live server state via public core APIs (e.g. Player::GetGroupInvite()); the packet itself is
// never seen here (opcode-as-signal only, handoff § 0).
class SignalTrigger : public Trigger
{
public:
    SignalTrigger(BotPlayerbotAI* botAI, std::string name) : Trigger(botAI, std::move(name)) { }

    bool IsActive() override;
};

#endif
