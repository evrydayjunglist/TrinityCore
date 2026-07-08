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

// AC reference: mod-playerbots-master/src/Script/PlayerbotCommandScript.cpp
// This fork: top-level `.playerbot` (singular). Master-alt control is `.playerbot bot`.

#include "ScriptMgr.h"
#include "BotPlayerbotAI.h"
#include "BotSessionMgr.h"
#include "Chat.h"
#include "ChatCommand.h"
#include "DB2Stores.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "PlayerbotMgr.h"
#include "Playerbots.h"
#include "PlayerbotsConfig.h"
#include "PlayerbotsDatabaseMgr.h"
#include "PlayerbotsMgr.h"
#include "QuestDef.h"
#include "RandomPlayerbotMgr.h"
#include "RBAC.h"
#include "Util.h"
#include "WorldSession.h"

namespace
{
// Short human label for a quest objective type, for the single-bot .playerbot status quest dump.
char const* QuestObjectiveTypeName(uint8 type)
{
    switch (type)
    {
        case QUEST_OBJECTIVE_MONSTER:          return "kill";
        case QUEST_OBJECTIVE_ITEM:             return "item";
        case QUEST_OBJECTIVE_GAMEOBJECT:       return "gameobject";
        case QUEST_OBJECTIVE_TALKTO:           return "talkto";
        case QUEST_OBJECTIVE_CURRENCY:         return "currency";
        case QUEST_OBJECTIVE_LEARNSPELL:       return "learnspell";
        case QUEST_OBJECTIVE_MIN_REPUTATION:   return "min-rep";
        case QUEST_OBJECTIVE_MAX_REPUTATION:   return "max-rep";
        case QUEST_OBJECTIVE_MONEY:            return "money";
        case QUEST_OBJECTIVE_PLAYERKILLS:      return "pvp-kills";
        case QUEST_OBJECTIVE_AREATRIGGER:      return "areatrigger";
        case QUEST_OBJECTIVE_HAVE_CURRENCY:    return "have-currency";
        case QUEST_OBJECTIVE_OBTAIN_CURRENCY:  return "obtain-currency";
        case QUEST_OBJECTIVE_INCREASE_REPUTATION: return "inc-rep";
        case QUEST_OBJECTIVE_AREA_TRIGGER_ENTER: return "area-enter";
        case QUEST_OBJECTIVE_AREA_TRIGGER_EXIT:  return "area-exit";
        default:                               return "other";
    }
}

char const* QuestStatusName(QuestStatus status)
{
    switch (status)
    {
        case QUEST_STATUS_NONE:       return "NONE";
        case QUEST_STATUS_COMPLETE:   return "COMPLETE";
        case QUEST_STATUS_INCOMPLETE: return "INCOMPLETE";
        case QUEST_STATUS_FAILED:     return "FAILED";
        case QUEST_STATUS_REWARDED:   return "REWARDED";
        default:                      return "?";
    }
}

// Rich per-bot dump for the single-bot filtered .playerbot status view (identity + real quest log
// + objective progress + ignored/low-prio flags). Kept out of the all-bots roster (hundreds of
// lines). See playerbots-rpg-active-questgiver-seeking-handoff.md §5.
void AppendSingleBotDetail(ChatHandler* handler, Player* bot, BotPlayerbotAI* botAI)
{
    // Identity: level, race/class, zone/area, map, position.
    char const* raceName = "?";
    if (ChrRacesEntry const* race = sChrRacesStore.LookupEntry(bot->GetRace()))
        raceName = race->Name[LOCALE_enUS];

    char const* className = "?";
    if (ChrClassesEntry const* cls = sChrClassesStore.LookupEntry(bot->GetClass()))
        className = cls->Name[LOCALE_enUS];

    char const* areaName = "?";
    if (AreaTableEntry const* area = sAreaTableStore.LookupEntry(bot->GetAreaId()))
        areaName = area->AreaName[LOCALE_enUS];

    handler->PSendSysMessage("    identity: L%u %s %s | zone %u area %u (%s) | map %u | pos %.1f %.1f %.1f",
        bot->GetLevel(), raceName, className, bot->GetZoneId(), bot->GetAreaId(), areaName,
        bot->GetMapId(), bot->GetPositionX(), bot->GetPositionY(), bot->GetPositionZ());

    // If the bot is actively seeking a quest giver, resolve the target guid to a name + distance
    // (NewRpgInfo::ToString only has the raw guid — no world access in that const helper).
    NewRpgInfo const& info = botAI->GetRpgInfo();
    if (info.GetStatus() == RPG_WANDER_NPC)
    {
        if (auto const* wander = std::get_if<NewRpgInfo::WanderNpc>(&info.data))
        {
            if (WorldObject* target = !wander->target.IsEmpty() ? ObjectAccessor::GetWorldObject(*bot, wander->target) : nullptr)
                handler->PSendSysMessage("    seeking: %s at %.1f yd", target->GetName().c_str(), bot->GetExactDist(*target));
            else
                handler->PSendSysMessage("    seeking: target %s (not in range/despawned)", wander->target.ToString().c_str());
        }
    }

    std::unordered_set<uint32> const& lowPrio = botAI->GetLowPriorityQuests();
    std::unordered_set<uint32> const& unactionable = botAI->GetUnactionableQuests();

    bool anyQuest = false;
    for (uint16 slot = 0; slot < MAX_QUEST_LOG_SIZE; ++slot)
    {
        uint32 const questId = bot->GetQuestSlotQuestId(slot);
        if (!questId)
            continue;

        anyQuest = true;
        Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
        QuestStatus const status = bot->GetQuestStatus(questId);

        std::string flags;
        if (unactionable.contains(questId))
            flags += " [ignored]";
        if (lowPrio.contains(questId))
            flags += " [low-prio]";
        if (!sObjectMgr->GetQuestPOIData(int32(questId)))
            flags += " [no-poi-data]";

        handler->PSendSysMessage("    quest %u \"%s\" | %s%s", questId,
            quest ? quest->GetLogTitle().c_str() : "?", QuestStatusName(status), flags.c_str());

        if (!quest)
            continue;

        for (QuestObjective const& objective : quest->Objectives)
        {
            bool const done = bot->IsQuestObjectiveComplete(slot, quest, objective);
            int32 const current = bot->GetQuestObjectiveData(questId, objective.ID);
            handler->PSendSysMessage("        obj %s objectId %d %d/%d%s",
                QuestObjectiveTypeName(objective.Type), objective.ObjectID,
                current, objective.Amount, done ? " (done)" : "");
        }
    }

    if (!anyQuest)
        handler->PSendSysMessage("    quest log: empty");
}
} // namespace

using namespace Trinity::ChatCommands;

class playerbots_commandscript : public CommandScript
{
public:
    playerbots_commandscript() : CommandScript("playerbots_commandscript") { }

    std::span<ChatCommandBuilder const> GetCommands() const override
    {
        static ChatCommandTable playerbotsBotCommandTable =
        {
            { "add",    HandlePlayerbotBotAddCommand,    rbac::RBAC_PERM_COMMAND_HELP, Console::No },
            { "remove", HandlePlayerbotBotRemoveCommand, rbac::RBAC_PERM_COMMAND_HELP, Console::No },
            { "list",   HandlePlayerbotBotListCommand,   rbac::RBAC_PERM_COMMAND_HELP, Console::No },
            { "logout", HandlePlayerbotBotLogoutCommand, rbac::RBAC_PERM_COMMAND_HELP, Console::No },
        };

        static ChatCommandTable playerbotsAccountCommandTable =
        {
            { "setKey",          HandlePlayerbotsNyiCommand, rbac::RBAC_PERM_COMMAND_HELP, Console::No },
            { "link",            HandlePlayerbotsNyiCommand, rbac::RBAC_PERM_COMMAND_HELP, Console::No },
            { "linkedAccounts",  HandlePlayerbotsNyiCommand, rbac::RBAC_PERM_COMMAND_HELP, Console::No },
            { "unlink",          HandlePlayerbotsNyiCommand, rbac::RBAC_PERM_COMMAND_HELP, Console::No },
        };

        static ChatCommandTable playerbotsRndbotCommandTable =
        {
            { "status", HandlePlayerbotRndbotStatusCommand, rbac::RBAC_PERM_COMMAND_GM, Console::No },
            { "start",  HandlePlayerbotRndbotStartCommand,  rbac::RBAC_PERM_COMMAND_GM, Console::No },
            { "stop",   HandlePlayerbotRndbotStopCommand,   rbac::RBAC_PERM_COMMAND_GM, Console::No },
        };

        static ChatCommandTable playerbotsCommandTable =
        {
            { "bot",    playerbotsBotCommandTable },
            { "rndbot", playerbotsRndbotCommandTable },
            { "account", playerbotsAccountCommandTable },
            // TC extensions (socketless GM bots on reserved accounts — Gates 3–4):
            { "login",  HandlePlayerbotsLoginCommand,     rbac::RBAC_PERM_COMMAND_GM,   Console::No },
            { "logout", HandlePlayerbotsLogoutCommand,    rbac::RBAC_PERM_COMMAND_GM,   Console::No },
            { "status", HandlePlayerbotsStatusCommand,    rbac::RBAC_PERM_COMMAND_HELP, Console::No },
        };

        static ChatCommandTable commandTable =
        {
            { "playerbot", playerbotsCommandTable },
        };
        return commandTable;
    }

    static bool HandlePlayerbotsStatusCommand(ChatHandler* handler, Optional<std::string> nameFilter)
    {
        if (!Playerbots::IsEnabled())
        {
            handler->SendSysMessage("Playerbots: module loaded, disabled (Playerbots.Enable = 0).");
            return true;
        }

        // Optional single-bot filter (the live roster can be hundreds of lines): an explicit name arg,
        // or failing that the GM's current target if it happens to be a bot. Empty = list every bot.
        std::string filterName;
        if (nameFilter && !nameFilter->empty())
            filterName = *nameFilter;
        else if (Player* selected = handler->getSelectedPlayer(); selected && sPlayerbotsMgr->GetPlayerbotAI(selected))
            filterName = selected->GetName();

        uint32 const maxBots = Playerbots::GetMaxActiveBots();
        uint32 const activeCount = static_cast<uint32>(sBotSessionMgr->GetActiveBotCount());
        if (filterName.empty())
            handler->PSendSysMessage("Playerbots: enabled. Active bots: %u/%u. Reserved accounts: %s.",
                activeCount, maxBots, Playerbots::GetReservedAccountPolicySummary().c_str());
        else
            handler->PSendSysMessage("Playerbots: status for '%s' (of %u active bots).", filterName.c_str(), activeCount);

        bool matched = false;
        sBotSessionMgr->ForEachActiveBot([&](WorldSession* session, ObjectGuid /*characterGuid*/)
        {
            Player* bot = session->GetPlayer();
            if (!bot)
                return;

            if (!filterName.empty() && !StringEqualI(bot->GetName(), filterName))
                return;

            matched = true;

            // Gate 10b: surface the RPG state machine + lifecycle counters per bot
            // (fork replacement for AC's whisper-based TellRpgStatusAction).
            if (BotPlayerbotAI* botAI = sPlayerbotsMgr->GetPlayerbotAI(bot))
            {
                NewRpgStatistic const& stats = botAI->GetRpgStatistics();
                // Counters are per-session (reset each login) — labelled so "accepted 0" is never
                // misread as "never quests" (the exact confusion the handoff §1 diagnosed).
                handler->PSendSysMessage("  - %s | %s | this session: accepted %u rewarded %u completed %u abandoned %u dropped %u | looted %u objects used %u talked %u | deaths %u revived %u",
                    bot->GetName().c_str(), botAI->GetRpgInfo().ToString().c_str(),
                    stats.questAccepted, stats.questRewarded, stats.questCompleted,
                    stats.questAbandoned, stats.questDropped,
                    stats.itemsLooted, stats.objectsUsed, stats.questNpcsTalkedTo,
                    stats.deaths, stats.revived);

                // Single-bot view only: dump the real quest log + identity (never in the terse
                // all-bots roster, which can be hundreds of lines).
                if (!filterName.empty())
                    AppendSingleBotDetail(handler, bot, botAI);
            }
            else
                handler->PSendSysMessage("  - %s", bot->GetName().c_str());
        });

        if (!filterName.empty() && !matched)
            handler->PSendSysMessage("Playerbots: no active bot named '%s' — target a bot in-game or pass its exact name.", filterName.c_str());

        if (filterName.empty())
            handler->SendSysMessage("Playerbots: GM — .playerbot login/logout. Master-alt — .playerbot bot add/remove/list/logout. Tip: '.playerbot status <name>' or target a bot to inspect just one.");

        return true;
    }

    static bool HandlePlayerbotsLoginCommand(ChatHandler* handler, std::string characterName)
    {
        return sBotSessionMgr->LoginCharacter(handler, characterName);
    }

    static bool HandlePlayerbotsLogoutCommand(ChatHandler* handler, Optional<std::string> characterName)
    {
        if (characterName)
            return sBotSessionMgr->LogoutCharacter(handler, &*characterName);
        return sBotSessionMgr->LogoutCharacter(handler, nullptr);
    }

    static bool HandlePlayerbotBotAddCommand(ChatHandler* handler, std::string characterName)
    {
        if (!Playerbots::IsEnabled())
        {
            handler->SendSysMessage("Playerbots: module loaded, disabled (Playerbots.Enable = 0).");
            return true;
        }

        Player* master = handler->GetSession() ? handler->GetSession()->GetPlayer() : nullptr;
        if (!master)
        {
            handler->SendSysMessage("Playerbots: .playerbot bot requires an in-game player session.");
            return false;
        }

        PlayerbotMgr* mgr = sPlayerbotsMgr->GetPlayerbotMgr(master);
        if (!mgr)
        {
            handler->SendSysMessage("Playerbots: PlayerbotMgr not found for your character.");
            return false;
        }

        return mgr->AddBot(characterName, handler);
    }

    static bool HandlePlayerbotBotRemoveCommand(ChatHandler* handler, std::string characterName)
    {
        if (!Playerbots::IsEnabled())
        {
            handler->SendSysMessage("Playerbots: module loaded, disabled (Playerbots.Enable = 0).");
            return true;
        }

        Player* master = handler->GetSession() ? handler->GetSession()->GetPlayer() : nullptr;
        if (!master)
        {
            handler->SendSysMessage("Playerbots: .playerbot bot requires an in-game player session.");
            return false;
        }

        PlayerbotMgr* mgr = sPlayerbotsMgr->GetPlayerbotMgr(master);
        if (!mgr)
        {
            handler->SendSysMessage("Playerbots: PlayerbotMgr not found for your character.");
            return false;
        }

        return mgr->RemoveBot(characterName, handler);
    }

    static bool HandlePlayerbotBotListCommand(ChatHandler* handler)
    {
        if (!Playerbots::IsEnabled())
        {
            handler->SendSysMessage("Playerbots: module loaded, disabled (Playerbots.Enable = 0).");
            return true;
        }

        Player* master = handler->GetSession() ? handler->GetSession()->GetPlayer() : nullptr;
        if (!master)
        {
            handler->SendSysMessage("Playerbots: .playerbot bot requires an in-game player session.");
            return false;
        }

        PlayerbotMgr* mgr = sPlayerbotsMgr->GetPlayerbotMgr(master);
        if (!mgr)
        {
            handler->SendSysMessage("Playerbots: PlayerbotMgr not found for your character.");
            return false;
        }

        mgr->ListBots(handler);
        return true;
    }

    static bool HandlePlayerbotBotLogoutCommand(ChatHandler* handler, Optional<std::string> characterName)
    {
        if (!Playerbots::IsEnabled())
        {
            handler->SendSysMessage("Playerbots: module loaded, disabled (Playerbots.Enable = 0).");
            return true;
        }

        Player* master = handler->GetSession() ? handler->GetSession()->GetPlayer() : nullptr;
        if (!master)
        {
            handler->SendSysMessage("Playerbots: .playerbot bot requires an in-game player session.");
            return false;
        }

        PlayerbotMgr* mgr = sPlayerbotsMgr->GetPlayerbotMgr(master);
        if (!mgr)
        {
            handler->SendSysMessage("Playerbots: PlayerbotMgr not found for your character.");
            return false;
        }

        if (characterName)
            return mgr->RemoveBot(*characterName, handler);

        mgr->LogoutAllBots();
        handler->SendSysMessage("Playerbots: logged out all your master-alt bots.");
        return true;
    }

    static bool HandlePlayerbotRndbotStatusCommand(ChatHandler* handler)
    {
        if (!Playerbots::IsEnabled())
        {
            handler->SendSysMessage("Playerbots: module loaded, disabled (Playerbots.Enable = 0).");
            return true;
        }

        handler->PSendSysMessage("Playerbots random bots: feature %s (MaxRandomBots=%u, MinRandomBots=%u).",
            Playerbots::IsRandomBotFeatureEnabled() ? "enabled" : "off",
            Playerbots::GetMaxRandomBots(), Playerbots::GetMinRandomBots());
        handler->PSendSysMessage("  Autologin: %s | DisabledWithoutRealPlayer: %s | Scheduler paused: %s",
            Playerbots::GetRandomBotAutologin() ? "yes" : "no",
            Playerbots::GetDisabledWithoutRealPlayer() ? "yes" : "no",
            sRandomPlayerbotMgr->IsSchedulerPaused() ? "yes" : "no");
        handler->PSendSysMessage("  Active random bots: %zu / target %zu",
            sRandomPlayerbotMgr->GetActiveRandomBotCount(), sRandomPlayerbotMgr->GetTargetRandomBotCount());
        handler->PSendSysMessage("  DB: %s", sPlayerbotsDatabaseMgr->GetStatusSummary().c_str());
        handler->PSendSysMessage("  Reserved accounts: %s", Playerbots::GetReservedAccountPolicySummary().c_str());
        return true;
    }

    static bool HandlePlayerbotRndbotStartCommand(ChatHandler* handler)
    {
        if (!Playerbots::IsEnabled())
        {
            handler->SendSysMessage("Playerbots: module loaded, disabled (Playerbots.Enable = 0).");
            return true;
        }

        if (!Playerbots::IsRandomBotFeatureEnabled())
        {
            handler->SendSysMessage("Playerbots: random bots off (set Playerbots.MaxRandomBots > 0).");
            return true;
        }

        sRandomPlayerbotMgr->SetSchedulerPaused(false);
        sRandomPlayerbotMgr->TriggerSchedulerPass();
        handler->SendSysMessage("Playerbots: random bot scheduler started (one pass requested).");
        return true;
    }

    static bool HandlePlayerbotRndbotStopCommand(ChatHandler* handler)
    {
        if (!Playerbots::IsEnabled())
        {
            handler->SendSysMessage("Playerbots: module loaded, disabled (Playerbots.Enable = 0).");
            return true;
        }

        sRandomPlayerbotMgr->SetSchedulerPaused(true);
        sRandomPlayerbotMgr->Shutdown();
        handler->SendSysMessage("Playerbots: random bots stopped and scheduler paused.");
        return true;
    }

    static bool HandlePlayerbotsNyiCommand(ChatHandler* handler)
    {
        if (!Playerbots::IsEnabled())
        {
            handler->SendSysMessage("Playerbots: module loaded, disabled (Playerbots.Enable = 0).");
            return true;
        }

        handler->SendSysMessage("Playerbots: not implemented.");
        return true;
    }
};

void AddPlayerbotsCommandscripts()
{
    new playerbots_commandscript();
}
