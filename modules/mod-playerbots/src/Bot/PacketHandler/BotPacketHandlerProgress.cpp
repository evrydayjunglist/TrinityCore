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
#include "Bot/Packet/BotLevelUpInfoPacket.h"
#include "Bot/Packet/BotLogXPGainPacket.h"
#include "Bot/Packet/BotCastFailedPacket.h"
#include "DBCEnums.h"
#include "Map.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "SharedDefines.h"
#include "SpellMgr.h"

namespace Playerbots::PacketHandler
{
void HandleLevelUpInfo(BotPlayerbotAI& ai, BotPlayerbotAI::QueuedSignal& signal)
{

        // Cleared unless Layer 2 OK. Enable=0 / mismatch: no poll dual.
        signal.Name.clear();
        ai.ClearPendingLevelUp();

        Playerbots::PacketParse::LevelUpInfoPayload parsed;
        if (!Playerbots::PacketParse::TryReadLevelUpInfo(*signal.Packet, parsed))
            return;

        // Sane level bounds: reject 0 / negative / beyond server-side strong max.
        if (parsed.Level < 1 || parsed.Level > int32(STRONG_MAX_LEVEL))
        {
            TC_LOG_ERROR("playerbots.packet",
                "BotPacketParse SMSG_LEVEL_UP_INFO Layer-2 Level out of bounds bot={} level={} verifiedBuild={}",
                ai.GetBot() ? ai.GetBot()->GetName() : "?",
                parsed.Level,
                Playerbots::PacketParse::VERIFIED_BUILD);
            return;
        }

        Player* bot = ai.GetBot();
        if (!bot || parsed.Level != int32(bot->GetLevel()))
        {
            TC_LOG_ERROR("playerbots.packet",
                "BotPacketParse SMSG_LEVEL_UP_INFO Layer-2 Level mismatch bot={} parsed={} live={} verifiedBuild={}",
                bot ? bot->GetName() : "?",
                parsed.Level,
                bot ? int32(bot->GetLevel()) : -1,
                Playerbots::PacketParse::VERIFIED_BUILD);
            return;
        }

        // Soft: negative talent slot deltas can occur on delevel; log only.
        if ((parsed.NumNewTalents < 0 || parsed.NumNewPvpTalentSlots < 0) &&
            Playerbots::GetLogLevel() >= 1)
        {
            TC_LOG_DEBUG("playerbots.packet",
                "BotPacketParse SMSG_LEVEL_UP_INFO soft negative talent deltas bot={} level={} talents={} pvpSlots={}",
                bot->GetName(),
                parsed.Level,
                parsed.NumNewTalents,
                parsed.NumNewPvpTalentSlots);
        }

        BotPlayerbotAI::PendingLevelUp stash;
        stash.Level = parsed.Level;
        ai.SetPendingLevelUp(std::move(stash));
        signal.Name = "levelup";

        if (Playerbots::GetLogLevel() >= 1)
            TC_LOG_DEBUG("playerbots.packet",
                "BotPacketParse SMSG_LEVEL_UP_INFO ok bot={} level={} healthDelta={} talents={} pvpSlots={}",
                bot->GetName(),
                parsed.Level,
                parsed.HealthDelta,
                parsed.NumNewTalents,
                parsed.NumNewPvpTalentSlots);
}

void HandleLogXpGain(BotPlayerbotAI& ai, BotPlayerbotAI::QueuedSignal& signal)
{

        // Cleared unless Layer 2 OK. Enable=0 / mismatch: no poll dual.
        // Do not hard-dual Amount vs GetXP() after apply (packet sent before SetXP).
        signal.Name.clear();
        ai.ClearPendingXpGain();

        Playerbots::PacketParse::LogXPGainPayload parsed;
        if (!Playerbots::PacketParse::TryReadLogXPGain(*signal.Packet, parsed))
            return;

        if (parsed.Reason != LOG_XP_REASON_KILL && parsed.Reason != LOG_XP_REASON_NO_KILL)
        {
            TC_LOG_ERROR("playerbots.packet",
                "BotPacketParse SMSG_LOG_XP_GAIN Layer-2 unknown Reason bot={} reason={} verifiedBuild={}",
                ai.GetBot() ? ai.GetBot()->GetName() : "?",
                uint32(parsed.Reason),
                Playerbots::PacketParse::VERIFIED_BUILD);
            return;
        }

        if (parsed.Amount <= 0 || parsed.Original < 0 || parsed.Original < parsed.Amount)
        {
            TC_LOG_ERROR("playerbots.packet",
                "BotPacketParse SMSG_LOG_XP_GAIN Layer-2 Amount/Original invalid bot={} amount={} original={} verifiedBuild={}",
                ai.GetBot() ? ai.GetBot()->GetName() : "?",
                parsed.Amount,
                parsed.Original,
                Playerbots::PacketParse::VERIFIED_BUILD);
            return;
        }

        if (!std::isfinite(parsed.GroupBonus))
        {
            TC_LOG_ERROR("playerbots.packet",
                "BotPacketParse SMSG_LOG_XP_GAIN Layer-2 GroupBonus not finite bot={} groupBonus={} verifiedBuild={}",
                ai.GetBot() ? ai.GetBot()->GetName() : "?",
                parsed.GroupBonus,
                Playerbots::PacketParse::VERIFIED_BUILD);
            return;
        }

        Player* bot = ai.GetBot();
        if (std::fabs(parsed.GroupBonus) > 100.0f && Playerbots::GetLogLevel() >= 1)
        {
            TC_LOG_DEBUG("playerbots.packet",
                "BotPacketParse SMSG_LOG_XP_GAIN soft absurd GroupBonus bot={} groupBonus={}",
                bot ? bot->GetName() : "?",
                parsed.GroupBonus);
        }

        // Soft Victim: KILL prefers non-empty map-local unit; NO_KILL empty is expected.
        // Map-local only — do not cold-query remote maps.
        if (parsed.Reason == LOG_XP_REASON_KILL)
        {
            if (parsed.Victim.IsEmpty())
            {
                if (Playerbots::GetLogLevel() >= 1)
                    TC_LOG_DEBUG("playerbots.packet",
                        "BotPacketParse SMSG_LOG_XP_GAIN soft empty Victim on KILL bot={}",
                        bot ? bot->GetName() : "?");
            }
            else
            {
                Map* map = bot ? bot->GetMap() : nullptr;
                Creature* victim = map ? map->GetCreature(parsed.Victim) : nullptr;
                if (!victim && Playerbots::GetLogLevel() >= 1)
                    TC_LOG_DEBUG("playerbots.packet",
                        "BotPacketParse SMSG_LOG_XP_GAIN soft Victim not map-local bot={} victim={}",
                        bot ? bot->GetName() : "?",
                        parsed.Victim.ToString());
            }
        }

        BotPlayerbotAI::PendingXpGain stash;
        stash.Amount = parsed.Amount;
        ai.SetPendingXpGain(std::move(stash));
        signal.Name = "xpgain";

        if (Playerbots::GetLogLevel() >= 1)
            TC_LOG_DEBUG("playerbots.packet",
                "BotPacketParse SMSG_LOG_XP_GAIN ok bot={} amount={} original={} reason={} victim={} groupBonus={}",
                bot ? bot->GetName() : "?",
                parsed.Amount,
                parsed.Original,
                uint32(parsed.Reason),
                parsed.Victim.ToString(),
                parsed.GroupBonus);
}

void HandleCastFailed(BotPlayerbotAI& ai, BotPlayerbotAI::QueuedSignal& signal)
{

        // Cleared unless Layer 2 OK. Enable=0: no poll dual.
        // Payload-only — do not invent GetCurrentSpell / pending-cast hard dual.
        signal.Name.clear();
        ai.ClearPendingCastFailed();

        Playerbots::PacketParse::CastFailedPayload parsed;
        if (!Playerbots::PacketParse::TryReadCastFailed(*signal.Packet, parsed))
            return;

        if (parsed.Reason == SPELL_CAST_OK || parsed.Reason == SPELL_FAILED_SUCCESS)
        {
            TC_LOG_ERROR("playerbots.packet",
                "BotPacketParse SMSG_CAST_FAILED Layer-2 success Reason on fail SMSG bot={} reason={} verifiedBuild={}",
                ai.GetBot() ? ai.GetBot()->GetName() : "?",
                parsed.Reason,
                Playerbots::PacketParse::VERIFIED_BUILD);
            return;
        }

        if (parsed.Reason < 0 || parsed.Reason > SPELL_FAILED_UNKNOWN)
        {
            TC_LOG_ERROR("playerbots.packet",
                "BotPacketParse SMSG_CAST_FAILED Layer-2 Reason out of range bot={} reason={} verifiedBuild={}",
                ai.GetBot() ? ai.GetBot()->GetName() : "?",
                parsed.Reason,
                Playerbots::PacketParse::VERIFIED_BUILD);
            return;
        }

        if (parsed.SpellID <= 0 || !sSpellMgr->GetSpellInfo(uint32(parsed.SpellID), DIFFICULTY_NONE))
        {
            TC_LOG_ERROR("playerbots.packet",
                "BotPacketParse SMSG_CAST_FAILED Layer-2 unknown SpellID bot={} spellId={} verifiedBuild={}",
                ai.GetBot() ? ai.GetBot()->GetName() : "?",
                parsed.SpellID,
                Playerbots::PacketParse::VERIFIED_BUILD);
            return;
        }

        Player* bot = ai.GetBot();
        // Soft FailedBy: empty expected for most Reasons; non-empty prefers map-local unit.
        if (!parsed.FailedBy.IsEmpty() && bot)
        {
            Unit* failedBy = ObjectAccessor::GetUnit(*bot, parsed.FailedBy);
            if (!failedBy && Playerbots::GetLogLevel() >= 1)
                TC_LOG_DEBUG("playerbots.packet",
                    "BotPacketParse SMSG_CAST_FAILED soft FailedBy not map-local bot={} failedBy={}",
                    bot->GetName(),
                    parsed.FailedBy.ToString());
        }

        BotPlayerbotAI::PendingCastFailed stash;
        stash.SpellID = parsed.SpellID;
        stash.Reason = parsed.Reason;
        ai.SetPendingCastFailed(std::move(stash));
        signal.Name = "cast failed";

        if (Playerbots::GetLogLevel() >= 1)
            TC_LOG_DEBUG("playerbots.packet",
                "BotPacketParse SMSG_CAST_FAILED ok bot={} spellId={} reason={} castId={} failedArg1={} failedArg2={} failedBy={}",
                bot ? bot->GetName() : "?",
                parsed.SpellID,
                parsed.Reason,
                parsed.CastID.ToString(),
                parsed.FailedArg1,
                parsed.FailedArg2,
                parsed.FailedBy.ToString());
}

} // namespace Playerbots::PacketHandler
