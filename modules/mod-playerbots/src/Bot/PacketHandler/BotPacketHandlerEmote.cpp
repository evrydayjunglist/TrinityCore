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
#include "Bot/Packet/BotEmotePacket.h"
#include "Bot/Packet/BotSTextEmotePacket.h"
#include "ObjectAccessor.h"
#include "Player.h"

namespace Playerbots::PacketHandler
{
void HandleTextEmote(BotPlayerbotAI& ai, BotPlayerbotAI::QueuedSignal& signal)
{

        // Cleared unless Layer 2 OK. Enable=0: no poll dual.
        // Payload-only — soft player/self/EmoteID filters; no invented pending-emote dual.
        signal.Name.clear();
        ai.ClearPendingReceiveEmote();

        Playerbots::PacketParse::STextEmotePayload parsed;
        if (!Playerbots::PacketParse::TryReadSTextEmote(*signal.Packet, parsed))
            return;

        Player* bot = ai.GetBot();
        if (parsed.SourceGUID.IsEmpty())
        {
            if (Playerbots::GetLogLevel() >= 1)
                TC_LOG_DEBUG("playerbots.packet",
                    "BotPacketParse SMSG_TEXT_EMOTE soft empty SourceGUID bot={}",
                    bot ? bot->GetName() : "?");
            return;
        }

        if (bot && parsed.SourceGUID == bot->GetGUID())
        {
            if (Playerbots::GetLogLevel() >= 1)
                TC_LOG_DEBUG("playerbots.packet",
                    "BotPacketParse SMSG_TEXT_EMOTE soft self-source skip bot={}",
                    bot->GetName());
            return;
        }

        if (!parsed.SourceGUID.IsPlayer())
        {
            if (Playerbots::GetLogLevel() >= 1)
                TC_LOG_DEBUG("playerbots.packet",
                    "BotPacketParse SMSG_TEXT_EMOTE soft non-player source skip bot={} source={}",
                    bot ? bot->GetName() : "?",
                    parsed.SourceGUID.ToString());
            return;
        }

        if (parsed.EmoteID == 0)
        {
            if (Playerbots::GetLogLevel() >= 1)
                TC_LOG_DEBUG("playerbots.packet",
                    "BotPacketParse SMSG_TEXT_EMOTE soft EmoteID=0 skip bot={}",
                    bot ? bot->GetName() : "?");
            return;
        }

        // Soft TargetGUID: empty is common; non-empty prefers map-local (no cold remote query).
        if (!parsed.TargetGUID.IsEmpty() && bot)
        {
            Unit* target = ObjectAccessor::GetUnit(*bot, parsed.TargetGUID);
            if (!target && Playerbots::GetLogLevel() >= 1)
                TC_LOG_DEBUG("playerbots.packet",
                    "BotPacketParse SMSG_TEXT_EMOTE soft TargetGUID not map-local bot={} target={}",
                    bot->GetName(),
                    parsed.TargetGUID.ToString());
        }

        BotPlayerbotAI::PendingReceiveEmote stash;
        stash.SourceGUID = parsed.SourceGUID;
        stash.EmoteID = uint32(parsed.EmoteID);
        stash.IsTextEmote = true;
        ai.SetPendingReceiveEmote(std::move(stash));
        signal.Name = "receive text emote";

        if (Playerbots::GetLogLevel() >= 1)
            TC_LOG_DEBUG("playerbots.packet",
                "BotPacketParse SMSG_TEXT_EMOTE ok bot={} source={} emoteId={} soundIndex={} target={}",
                bot ? bot->GetName() : "?",
                parsed.SourceGUID.ToString(),
                parsed.EmoteID,
                parsed.SoundIndex,
                parsed.TargetGUID.ToString());
}

void HandleEmote(BotPlayerbotAI& ai, BotPlayerbotAI::QueuedSignal& signal)
{

        // Cleared unless Layer 2 OK. Enable=0: no poll dual.
        // Payload-only — soft player/self/ONESHOT_NONE filters (AC NPC pre-filter intent).
        signal.Name.clear();
        ai.ClearPendingReceiveEmote();

        Playerbots::PacketParse::EmotePayload parsed;
        if (!Playerbots::PacketParse::TryReadEmote(*signal.Packet, parsed))
            return;

        Player* bot = ai.GetBot();
        if (parsed.Guid.IsEmpty())
        {
            if (Playerbots::GetLogLevel() >= 1)
                TC_LOG_DEBUG("playerbots.packet",
                    "BotPacketParse SMSG_EMOTE soft empty Guid bot={}",
                    bot ? bot->GetName() : "?");
            return;
        }

        if (bot && parsed.Guid == bot->GetGUID())
        {
            if (Playerbots::GetLogLevel() >= 1)
                TC_LOG_DEBUG("playerbots.packet",
                    "BotPacketParse SMSG_EMOTE soft self-source skip bot={}",
                    bot->GetName());
            return;
        }

        // Match AC HandleBotOutgoingPacket: skip NPC oneshots without Layer-2 ERROR.
        if (!parsed.Guid.IsPlayer())
        {
            if (Playerbots::GetLogLevel() >= 1)
                TC_LOG_DEBUG("playerbots.packet",
                    "BotPacketParse SMSG_EMOTE soft non-player source skip bot={} source={}",
                    bot ? bot->GetName() : "?",
                    parsed.Guid.ToString());
            return;
        }

        if (parsed.EmoteID == EMOTE_ONESHOT_NONE)
        {
            if (Playerbots::GetLogLevel() >= 1)
                TC_LOG_DEBUG("playerbots.packet",
                    "BotPacketParse SMSG_EMOTE soft ONESHOT_NONE skip bot={}",
                    bot ? bot->GetName() : "?");
            return;
        }

        BotPlayerbotAI::PendingReceiveEmote stash;
        stash.SourceGUID = parsed.Guid;
        stash.EmoteID = parsed.EmoteID;
        stash.IsTextEmote = false;
        ai.SetPendingReceiveEmote(std::move(stash));
        signal.Name = "receive emote";

        if (Playerbots::GetLogLevel() >= 1)
            TC_LOG_DEBUG("playerbots.packet",
                "BotPacketParse SMSG_EMOTE ok bot={} source={} emoteId={} kits={} sequenceVariation={}",
                bot ? bot->GetName() : "?",
                parsed.Guid.ToString(),
                parsed.EmoteID,
                parsed.SpellVisualKitIDs.size(),
                parsed.SequenceVariation);
}

} // namespace Playerbots::PacketHandler
