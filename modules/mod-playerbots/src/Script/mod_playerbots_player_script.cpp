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
#include "ObjectAccessor.h"
#include "Player.h"
#include "PlayerbotMgr.h"
#include "Playerbots.h"
#include "PlayerbotsConfig.h"
#include "RandomPlayerbotMgr.h"
#include "WorldSession.h"

// Bot lifecycle: session registration + PlayerbotsMgr AI registry (Gate 5).
// Human lifecycle: PlayerbotMgr per master (Gate 7).
// Do not use Player::SetAI / UnitAI for socketless bots — core refuses SetAI on IsBotSession() players.

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

            ObjectGuid const masterGuid = sBotSessionMgr->GetMasterGuidForBot(player->GetGUID());
            if (!masterGuid.IsEmpty())
                if (Player* master = ObjectAccessor::FindConnectedPlayer(masterGuid))
                    if (PlayerbotMgr* mgr = sPlayerbotsMgr->GetPlayerbotMgr(master))
                        mgr->OnBotLogin(player);
            return;
        }

        sPlayerbotsMgr->AddPlayerbotData(player, false);
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
