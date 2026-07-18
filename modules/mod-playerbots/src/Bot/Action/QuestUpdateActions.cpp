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

#include "QuestUpdateActions.h"
#include "BotPlayerbotAI.h"
#include "Log.h"
#include "Opcodes.h"
#include "Player.h"
#include "PlayerbotsConfig.h"
#include "QuestPackets.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include <sstream>

bool QuestUpdateCompleteAction::IsUseful()
{
    return _botAI && _botAI->GetBot() && _botAI->GetPendingQuestUpdateComplete().has_value();
}

bool QuestUpdateCompleteAction::Execute(Event /*event*/)
{
    if (!_botAI)
        return false;

    std::optional<BotPlayerbotAI::PendingQuestUpdateComplete> const pending =
        _botAI->GetPendingQuestUpdateComplete();
    _botAI->ClearPendingQuestUpdateComplete();
    if (!pending)
        return false;

    Player* bot = _botAI->GetBot();
    if (!bot)
        return false;

    if (!_botAI->HasMaster())
        return true;

    std::ostringstream out;
    out << "Quest completed " << pending->QuestID;
    bool const told = _botAI->TellMaster(out.str());
    if (Playerbots::GetLogLevel() >= 1)
    {
        Player* master = _botAI->GetMaster();
        TC_LOG_DEBUG("playerbots", "QuestUpdateCompleteAction bot={} master={} text='{}' ok={}",
            bot->GetName(),
            master ? master->GetName() : "none",
            out.str(),
            told ? "yes" : "no");
    }

    return true;
}

bool QuestUpdateAddKillAction::IsUseful()
{
    return _botAI && _botAI->GetBot() && _botAI->GetPendingQuestUpdateAddKill().has_value();
}

bool QuestUpdateAddKillAction::Execute(Event /*event*/)
{
    if (!_botAI)
        return false;

    std::optional<BotPlayerbotAI::PendingQuestUpdateAddKill> const pending =
        _botAI->GetPendingQuestUpdateAddKill();
    _botAI->ClearPendingQuestUpdateAddKill();
    if (!pending)
        return false;

    Player* bot = _botAI->GetBot();
    if (!bot)
        return false;

    if (!_botAI->HasMaster())
        return true;

    std::ostringstream out;
    out << "quest " << pending->QuestID << " " << pending->Count << "/" << pending->Required
        << " obj=" << pending->ObjectID;
    bool const told = _botAI->TellMaster(out.str());
    if (Playerbots::GetLogLevel() >= 1)
    {
        Player* master = _botAI->GetMaster();
        TC_LOG_DEBUG("playerbots", "QuestUpdateAddKillAction bot={} master={} text='{}' ok={}",
            bot->GetName(),
            master ? master->GetName() : "none",
            out.str(),
            told ? "yes" : "no");
    }

    return true;
}

namespace
{
bool ResolveMasterSharedQuestConfirm(BotPlayerbotAI* botAI, Player*& bot, Player*& master, uint32& questId)
{
    if (!botAI)
        return false;

    bot = botAI->GetBot();
    master = botAI->GetMaster();
    if (!bot || !master || !bot->GetSession())
        return false;

    auto const& pending = botAI->GetPendingQuestConfirm();
    if (pending && pending->QuestID)
        questId = pending->QuestID;
    else
        questId = bot->GetSharedQuestID();

    if (!questId)
        return false;

    if (bot->GetSharedQuestID() != questId)
        return false;

    if (bot->GetPlayerSharingQuest() != master->GetGUID())
        return false;

    return true;
}
}

bool ConfirmQuestAction::IsUseful()
{
    Player* bot = nullptr;
    Player* master = nullptr;
    uint32 questId = 0;
    return ResolveMasterSharedQuestConfirm(_botAI, bot, master, questId);
}

bool ConfirmQuestAction::Execute(Event /*event*/)
{
    Player* bot = nullptr;
    Player* master = nullptr;
    uint32 questId = 0;
    if (!ResolveMasterSharedQuestConfirm(_botAI, bot, master, questId))
        return false;

    // Midnight: CMSG_QUEST_CONFIRM_ACCEPT → HandleQuestConfirmAccept.
    // Do not paste AC ConfirmQuestAction AddQuest bypass / QueuePacket / FormatQuest.
    WorldPacket packet(CMSG_QUEST_CONFIRM_ACCEPT);
    WorldPackets::Quest::QuestConfirmAccept confirm(std::move(packet));
    confirm.QuestID = int32(questId);
    bot->GetSession()->HandleQuestConfirmAccept(confirm);

    _botAI->ClearPendingQuestConfirm();

    if (_botAI->HasMaster())
    {
        std::ostringstream out;
        out << "confirm quest " << questId;
        _botAI->TellMaster(out.str());
    }

    if (Playerbots::GetLogLevel() >= 1)
        TC_LOG_DEBUG("playerbots", "ConfirmQuestAction bot={} master={} questId={}",
            bot->GetName(),
            master->GetName(),
            questId);

    return true;
}
