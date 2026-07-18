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
#include "Bot/Packet/BotQuestUpdateCompletePacket.h"
#include "Bot/Packet/BotQuestUpdateAddCreditPacket.h"
#include "Bot/Packet/BotQuestConfirmAcceptPacket.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "Player.h"

namespace Playerbots::PacketHandler
{
void HandleQuestUpdateComplete(BotPlayerbotAI& ai, BotPlayerbotAI::QueuedSignal& signal)
{

        // Cleared unless Layer 2 OK (active / COMPLETE). Enable=0: no poll dual.
        signal.Name.clear();
        ai.ClearPendingQuestUpdateComplete();

        Playerbots::PacketParse::QuestUpdateCompletePayload parsed;
        if (!Playerbots::PacketParse::TryReadQuestUpdateComplete(*signal.Packet, parsed))
            return;

        if (!parsed.QuestID)
        {
            TC_LOG_ERROR("playerbots.packet",
                "BotPacketParse SMSG_QUEST_UPDATE_COMPLETE Layer-2 QuestID=0 bot={} verifiedBuild={}",
                ai.GetBot() ? ai.GetBot()->GetName() : "?",
                Playerbots::PacketParse::VERIFIED_BUILD);
            return;
        }

        Player* bot = ai.GetBot();
        if (!bot)
            return;

        Quest const* quest = sObjectMgr->GetQuestTemplate(uint32(parsed.QuestID));
        if (!quest)
        {
            if (Playerbots::GetLogLevel() >= 1)
                TC_LOG_DEBUG("playerbots.packet",
                    "BotPacketParse SMSG_QUEST_UPDATE_COMPLETE soft missing template bot={} questId={}",
                    bot->GetName(),
                    parsed.QuestID);
            return;
        }

        bool const activeOrComplete = bot->IsActiveQuest(uint32(parsed.QuestID)) ||
            bot->GetQuestStatus(uint32(parsed.QuestID)) == QUEST_STATUS_COMPLETE;
        if (!activeOrComplete)
        {
            TC_LOG_ERROR("playerbots.packet",
                "BotPacketParse SMSG_QUEST_UPDATE_COMPLETE Layer-2 not active/complete bot={} questId={} status={} verifiedBuild={}",
                bot->GetName(),
                parsed.QuestID,
                uint32(bot->GetQuestStatus(uint32(parsed.QuestID))),
                Playerbots::PacketParse::VERIFIED_BUILD);
            return;
        }

        if (Playerbots::GetLogLevel() >= 1)
            TC_LOG_DEBUG("playerbots.packet",
                "BotPacketParse SMSG_QUEST_UPDATE_COMPLETE ok bot={} questId={} hideCredit={}",
                bot->GetName(),
                parsed.QuestID,
                parsed.HideCreditMessage ? "yes" : "no");

        BotPlayerbotAI::PendingQuestUpdateComplete stash;
        stash.QuestID = parsed.QuestID;
        ai.SetPendingQuestUpdateComplete(std::move(stash));
        signal.Name = "quest update complete";
}

void HandleQuestUpdateAddCredit(BotPlayerbotAI& ai, BotPlayerbotAI::QueuedSignal& signal)
{

        // Cleared unless Layer 2 OK (IsActiveQuest). Keep AC signal "quest update add kill".
        signal.Name.clear();
        ai.ClearPendingQuestUpdateAddKill();

        Playerbots::PacketParse::QuestUpdateAddCreditPayload parsed;
        if (!Playerbots::PacketParse::TryReadQuestUpdateAddCredit(*signal.Packet, parsed))
            return;

        if (!parsed.QuestID)
        {
            TC_LOG_ERROR("playerbots.packet",
                "BotPacketParse SMSG_QUEST_UPDATE_ADD_CREDIT Layer-2 QuestID=0 bot={} verifiedBuild={}",
                ai.GetBot() ? ai.GetBot()->GetName() : "?",
                Playerbots::PacketParse::VERIFIED_BUILD);
            return;
        }

        Player* bot = ai.GetBot();
        if (!bot)
            return;

        Quest const* quest = sObjectMgr->GetQuestTemplate(uint32(parsed.QuestID));
        if (!quest)
        {
            if (Playerbots::GetLogLevel() >= 1)
                TC_LOG_DEBUG("playerbots.packet",
                    "BotPacketParse SMSG_QUEST_UPDATE_ADD_CREDIT soft missing template bot={} questId={}",
                    bot->GetName(),
                    parsed.QuestID);
            return;
        }

        if (!bot->IsActiveQuest(uint32(parsed.QuestID)))
        {
            TC_LOG_ERROR("playerbots.packet",
                "BotPacketParse SMSG_QUEST_UPDATE_ADD_CREDIT Layer-2 not active bot={} questId={} verifiedBuild={}",
                bot->GetName(),
                parsed.QuestID,
                Playerbots::PacketParse::VERIFIED_BUILD);
            return;
        }

        if (parsed.Required > 0 && parsed.Count > parsed.Required && Playerbots::GetLogLevel() >= 1)
            TC_LOG_DEBUG("playerbots.packet",
                "BotPacketParse SMSG_QUEST_UPDATE_ADD_CREDIT soft Count>Required bot={} questId={} count={} required={}",
                bot->GetName(),
                parsed.QuestID,
                parsed.Count,
                parsed.Required);

        if (!parsed.ObjectID && Playerbots::GetLogLevel() >= 1)
            TC_LOG_DEBUG("playerbots.packet",
                "BotPacketParse SMSG_QUEST_UPDATE_ADD_CREDIT soft ObjectID=0 bot={} questId={}",
                bot->GetName(),
                parsed.QuestID);

        if (Playerbots::GetLogLevel() >= 1)
            TC_LOG_DEBUG("playerbots.packet",
                "BotPacketParse SMSG_QUEST_UPDATE_ADD_CREDIT ok bot={} questId={} objectId={} count={}/{} type={} victim={}",
                bot->GetName(),
                parsed.QuestID,
                parsed.ObjectID,
                parsed.Count,
                parsed.Required,
                parsed.ObjectiveType,
                parsed.VictimGUID.ToString());

        BotPlayerbotAI::PendingQuestUpdateAddKill stash;
        stash.QuestID = parsed.QuestID;
        stash.ObjectID = parsed.ObjectID;
        stash.Count = parsed.Count;
        stash.Required = parsed.Required;
        ai.SetPendingQuestUpdateAddKill(std::move(stash));
        signal.Name = "quest update add kill";
}

void HandleQuestConfirmAccept(BotPlayerbotAI& ai, BotPlayerbotAI::QueuedSignal& signal)
{

        // Cleared unless Layer 2 OK (shared quest + InitiatedBy). Enable=0: poll twin may fire.
        signal.Name.clear();
        ai.ClearPendingQuestConfirm();

        Playerbots::PacketParse::QuestConfirmAcceptPayload parsed;
        if (!Playerbots::PacketParse::TryReadQuestConfirmAccept(*signal.Packet, parsed))
            return;

        if (!parsed.QuestID)
        {
            TC_LOG_ERROR("playerbots.packet",
                "BotPacketParse SMSG_QUEST_CONFIRM_ACCEPT Layer-2 QuestID=0 bot={} verifiedBuild={}",
                ai.GetBot() ? ai.GetBot()->GetName() : "?",
                Playerbots::PacketParse::VERIFIED_BUILD);
            return;
        }

        Player* bot = ai.GetBot();
        if (!bot)
            return;

        Quest const* quest = sObjectMgr->GetQuestTemplate(parsed.QuestID);
        if (!quest)
        {
            TC_LOG_ERROR("playerbots.packet",
                "BotPacketParse SMSG_QUEST_CONFIRM_ACCEPT Layer-2 missing template bot={} questId={} verifiedBuild={}",
                bot->GetName(),
                parsed.QuestID,
                Playerbots::PacketParse::VERIFIED_BUILD);
            return;
        }

        if (bot->GetSharedQuestID() != parsed.QuestID)
        {
            TC_LOG_ERROR("playerbots.packet",
                "BotPacketParse SMSG_QUEST_CONFIRM_ACCEPT Layer-2 SharedQuestID mismatch bot={} parsed={} live={} verifiedBuild={}",
                bot->GetName(),
                parsed.QuestID,
                bot->GetSharedQuestID(),
                Playerbots::PacketParse::VERIFIED_BUILD);
            return;
        }

        if (parsed.InitiatedBy != bot->GetPlayerSharingQuest())
        {
            TC_LOG_ERROR("playerbots.packet",
                "BotPacketParse SMSG_QUEST_CONFIRM_ACCEPT Layer-2 InitiatedBy mismatch bot={} parsed={} live={} verifiedBuild={}",
                bot->GetName(),
                parsed.InitiatedBy.ToString(),
                bot->GetPlayerSharingQuest().ToString(),
                Playerbots::PacketParse::VERIFIED_BUILD);
            return;
        }

        // Soft: InitiatedBy player + map-local when practical — no cold remote query.
        if (!parsed.InitiatedBy.IsPlayer())
        {
            if (Playerbots::GetLogLevel() >= 1)
                TC_LOG_DEBUG("playerbots.packet",
                    "BotPacketParse SMSG_QUEST_CONFIRM_ACCEPT soft non-player InitiatedBy bot={} initiatedBy={}",
                    bot->GetName(),
                    parsed.InitiatedBy.ToString());
        }
        else if (Player* sharer = ObjectAccessor::GetPlayer(*bot, parsed.InitiatedBy); !sharer)
        {
            if (Playerbots::GetLogLevel() >= 1)
                TC_LOG_DEBUG("playerbots.packet",
                    "BotPacketParse SMSG_QUEST_CONFIRM_ACCEPT soft InitiatedBy not map-local bot={} initiatedBy={}",
                    bot->GetName(),
                    parsed.InitiatedBy.ToString());
        }
        else if (!bot->IsInSameRaidWith(sharer) && Playerbots::GetLogLevel() >= 1)
        {
            TC_LOG_DEBUG("playerbots.packet",
                "BotPacketParse SMSG_QUEST_CONFIRM_ACCEPT soft not same group/raid bot={} sharer={}",
                bot->GetName(),
                sharer->GetName());
        }

        Player* master = ai.GetMaster();
        bool const masterSharer = master && parsed.InitiatedBy == master->GetGUID();

        if (Playerbots::GetLogLevel() >= 1)
            TC_LOG_DEBUG("playerbots.packet",
                "BotPacketParse SMSG_QUEST_CONFIRM_ACCEPT ok bot={} questId={} initiatedBy={} title='{}' masterSharer={}",
                bot->GetName(),
                parsed.QuestID,
                parsed.InitiatedBy.ToString(),
                parsed.QuestTitle,
                masterSharer ? "yes" : "no");

        // V1 FollowMaster: only enqueue confirm signal for master sharer.
        if (!masterSharer)
            return;

        BotPlayerbotAI::PendingQuestConfirm stash;
        stash.QuestID = parsed.QuestID;
        stash.InitiatedBy = parsed.InitiatedBy;
        ai.SetPendingQuestConfirm(std::move(stash));
        signal.Name = "confirm quest";
}

} // namespace Playerbots::PacketHandler
