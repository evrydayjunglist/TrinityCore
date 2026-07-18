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

#ifndef TRINITY_BOT_PACKET_SIGNAL_H
#define TRINITY_BOT_PACKET_SIGNAL_H

#include "BotPlayerbotAI.h"
#include "Define.h"

namespace Playerbots::PacketHandler
{
using HandlerFn = void (*)(BotPlayerbotAI& ai, BotPlayerbotAI::QueuedSignal& signal);
using ClearPendingFn = void (BotPlayerbotAI::*)();

// Registry row matching parent handoff schema:
// opcode → { signalName, copiesPayload?, handler, clearOnNoPayload?, clearPending? }
struct PacketSignalEntry
{
    char const* SignalName = nullptr;
    bool CopiesPayload = false;
    HandlerFn Handler = nullptr;
    // When PayloadParse is off (Packet == nullptr): if true, clear Name (+ optional stash).
    bool ClearSignalOnNoPayload = false;
    ClearPendingFn ClearPending = nullptr;
};

PacketSignalEntry const* LookupPacketSignal(uint32 opcode);

void HandlePartyInvite(BotPlayerbotAI& ai, BotPlayerbotAI::QueuedSignal& signal);
void HandleGuildInvite(BotPlayerbotAI& ai, BotPlayerbotAI::QueuedSignal& signal);
void HandleResurrectRequest(BotPlayerbotAI& ai, BotPlayerbotAI::QueuedSignal& signal);
void HandlePetitionShowSignatures(BotPlayerbotAI& ai, BotPlayerbotAI::QueuedSignal& signal);
void HandleDuelRequested(BotPlayerbotAI& ai, BotPlayerbotAI::QueuedSignal& signal);

void HandleBuyFailed(BotPlayerbotAI& ai, BotPlayerbotAI::QueuedSignal& signal);
void HandleInventoryChangeFailure(BotPlayerbotAI& ai, BotPlayerbotAI::QueuedSignal& signal);
void HandleTradeStatus(BotPlayerbotAI& ai, BotPlayerbotAI::QueuedSignal& signal);
void HandleTradeUpdated(BotPlayerbotAI& ai, BotPlayerbotAI::QueuedSignal& signal);

void HandleLootResponse(BotPlayerbotAI& ai, BotPlayerbotAI::QueuedSignal& signal);
void HandleItemPushResult(BotPlayerbotAI& ai, BotPlayerbotAI::QueuedSignal& signal);
void HandleLootRollWon(BotPlayerbotAI& ai, BotPlayerbotAI::QueuedSignal& signal);
void HandleStartLootRoll(BotPlayerbotAI& ai, BotPlayerbotAI::QueuedSignal& signal);

void HandleGroupNewLeader(BotPlayerbotAI& ai, BotPlayerbotAI::QueuedSignal& signal);
void HandleGroupDestroyed(BotPlayerbotAI& ai, BotPlayerbotAI::QueuedSignal& signal);
void HandleMoveSetRunSpeed(BotPlayerbotAI& ai, BotPlayerbotAI::QueuedSignal& signal);
void HandlePartyCommandResult(BotPlayerbotAI& ai, BotPlayerbotAI::QueuedSignal& signal);

void HandleLevelUpInfo(BotPlayerbotAI& ai, BotPlayerbotAI::QueuedSignal& signal);
void HandleLogXpGain(BotPlayerbotAI& ai, BotPlayerbotAI::QueuedSignal& signal);
void HandleCastFailed(BotPlayerbotAI& ai, BotPlayerbotAI::QueuedSignal& signal);

void HandleTextEmote(BotPlayerbotAI& ai, BotPlayerbotAI::QueuedSignal& signal);
void HandleEmote(BotPlayerbotAI& ai, BotPlayerbotAI::QueuedSignal& signal);

void HandleLfgRoleCheckUpdate(BotPlayerbotAI& ai, BotPlayerbotAI::QueuedSignal& signal);
void HandleLfgProposalUpdate(BotPlayerbotAI& ai, BotPlayerbotAI::QueuedSignal& signal);

void HandleQuestUpdateComplete(BotPlayerbotAI& ai, BotPlayerbotAI::QueuedSignal& signal);
void HandleQuestUpdateAddCredit(BotPlayerbotAI& ai, BotPlayerbotAI::QueuedSignal& signal);
void HandleQuestConfirmAccept(BotPlayerbotAI& ai, BotPlayerbotAI::QueuedSignal& signal);
} // namespace Playerbots::PacketHandler

#endif // TRINITY_BOT_PACKET_SIGNAL_H
