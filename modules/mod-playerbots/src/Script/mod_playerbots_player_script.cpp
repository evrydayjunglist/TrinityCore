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

#include "ScriptMgr.h"
#include "BotPlayerbotAI.h"
#include "BotSessionMgr.h"
#include "Mgr/Talent/BotTalentMgr.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "PlayerbotMgr.h"
#include "Playerbots.h"
#include "PlayerbotsConfig.h"
#include "RandomPlayerbotMgr.h"
#include "Random.h"
#include "World.h"
#include "WorldSession.h"
#include <algorithm>

// Bot lifecycle: session registration + PlayerbotsMgr AI registry (Gate 5).
// Human lifecycle: PlayerbotMgr per master (Gate 7).
// Do not use Player::SetAI / UnitAI for socketless bots — core refuses SetAI on IsBotSession() players.

namespace
{
// Gate 10 — applied once, the first time a random bot logs in on its freshly-created
// (level 1) character; matches AC's RandomizeFirst() level roll in spirit, without AC's
// mass PlayerbotFactory gear/talent randomization (out of Gate 10 scope — see roadmap).
void ApplyRandomBotStartingLevel(Player* bot)
{
    if (!bot || bot->GetLevel() != 1)
        return;

    uint32 const serverMaxLevel = sWorld->getIntConfig(CONFIG_MAX_PLAYER_LEVEL);
    uint32 maxLevel = Playerbots::GetRandomBotMaxLevel();
    if (!maxLevel || maxLevel > serverMaxLevel)
        maxLevel = serverMaxLevel;

    uint32 const minLevel = std::min(Playerbots::GetRandomBotMinLevel(), maxLevel);

    uint32 level;
    if (Playerbots::GetDisableRandomLevels())
        level = std::clamp(Playerbots::GetRandombotStartingLevel(), 1u, maxLevel);
    else
        level = minLevel >= maxLevel ? maxLevel : urand(minLevel, maxLevel);

    if (level <= 1)
        return;

    bot->GiveLevel(static_cast<uint8>(level));
    bot->InitTalentForLevel();
    bot->SetXP(0);
}
}

class mod_playerbots_player_script : public PlayerScript
{
public:
    mod_playerbots_player_script() : PlayerScript("mod_playerbots_player_script") { }

    void OnLogin(Player* player, bool /*firstLogin*/) override
    {
        if (!Playerbots::IsEnabled())
            return;

        WorldSession* session = player->GetSession();
        if (!session)
            return;

        if (session->IsBotSession())
        {
            sBotSessionMgr->RegisterSession(session, player->GetGUID());
            sPlayerbotsMgr->AddPlayerbotData(player, true);

            if (sRandomPlayerbotMgr->IsRandomBot(player->GetGUID()))
                ApplyRandomBotStartingLevel(player);

            // Gate 13 — after GiveLevel so TraitMgr currencies/ranks match bot level.
            // Covers master-alt and random bots (PlayerbotMgr::OnBotLogin is master-alt only).
            if (Playerbots::GetTalentApplyOnLogin())
                BotTalentMgr::EnsureSpecAndStarterTraits(player);

            ObjectGuid const masterGuid = sBotSessionMgr->GetMasterGuidForBot(player->GetGUID());
            if (!masterGuid.IsEmpty())
                if (Player* master = ObjectAccessor::FindConnectedPlayer(masterGuid))
                    if (PlayerbotMgr* mgr = sPlayerbotsMgr->GetPlayerbotMgr(master))
                        mgr->OnBotLogin(player);
            return;
        }

        sPlayerbotsMgr->AddPlayerbotData(player, false);
    }

    void OnLevelChanged(Player* player, uint8 /*oldLevel*/) override
    {
        if (!Playerbots::IsEnabled() || !player)
            return;

        WorldSession* session = player->GetSession();
        if (!session || !session->IsBotSession())
            return;

        if (!Playerbots::GetTalentApplyOnLevelUp())
            return;

        BotTalentMgr::EnsureSpecAndStarterTraits(player, true);
    }

    void OnGiveXP(Player* player, uint32& amount, Unit* /*victim*/) override
    {
        if (!Playerbots::IsEnabled() || !player)
            return;

        if (!sRandomPlayerbotMgr->IsRandomBot(player->GetGUID()))
            return;

        // AC semantics (see PlayerbotAIConfig randomBotFixedLevel / XpGainAction.cpp comment):
        // fixed-level bots gain no XP at all; otherwise scale by the configured rate.
        if (Playerbots::GetRandomBotFixedLevel())
        {
            amount = 0;
            return;
        }

        float const rate = Playerbots::GetRandomBotXPRate();
        if (rate != 1.0f)
            amount = static_cast<uint32>(amount * rate);
    }

    void OnLogout(Player* player) override
    {
        if (!Playerbots::IsEnabled())
            return;

        WorldSession* session = player->GetSession();
        if (!session)
            return;

        if (session->IsBotSession())
        {
            ObjectGuid const masterGuid = sBotSessionMgr->GetMasterGuidForBot(player->GetGUID());
            if (!masterGuid.IsEmpty())
                if (Player* master = ObjectAccessor::FindConnectedPlayer(masterGuid))
                    if (PlayerbotMgr* mgr = sPlayerbotsMgr->GetPlayerbotMgr(master))
                        mgr->OnBotLogout(player->GetGUID());

            if (sRandomPlayerbotMgr->IsRandomBot(player->GetGUID()))
                sRandomPlayerbotMgr->OnRandomBotLoggedOut(player->GetGUID());

            sPlayerbotsMgr->RemovePlayerbotData(player, true);
            sBotSessionMgr->UnregisterSession(session);
            return;
        }

        if (PlayerbotMgr* mgr = sPlayerbotsMgr->GetPlayerbotMgr(player))
            mgr->LogoutAllBots();

        sPlayerbotsMgr->RemovePlayerbotData(player, false);
    }

    void OnPlayerAfterUpdate(Player* player, uint32 diff) override
    {
        if (BotPlayerbotAI* ai = GET_PLAYERBOT_AI(player))
            ai->UpdateAI(diff);
    }
};

void AddSC_mod_playerbots_player_script()
{
    new mod_playerbots_player_script();
}
