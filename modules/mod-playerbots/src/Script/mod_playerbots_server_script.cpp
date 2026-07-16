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
#include "Player.h"
#include "Playerbots.h"
#include "PlayerbotsConfig.h"
#include "WorldPacket.h"
#include "WorldSession.h"

// Bot outbound-packet observation (playerbots-bot-packet-observation-handoff.md § 5.1). TC's core
// already ships ServerScript::OnPacketSend and fires it for every real-player send; the fork's
// one-line core seam (WorldSession::SendPacket bot branch) now also fires it for the packets that
// would otherwise be silently dropped for socketless bot sessions. This observer routes those to
// the bot's AI so it can react to the SMSG the server generated for it. AC uses a dedicated
// OnPlayerbotPacketSent hook (mod-playerbots-master Script/Playerbots.cpp:434); reusing TC's
// generic ServerScript hook avoids adding a parallel bespoke core hook type for zero capability.
//
// Threading: OnPacketSend runs on arbitrary map/World sender threads. This observer does nothing
// but a cheap enabled/bot test + hand-off to HandleBotOutgoingPacket (a static registry test +
// guarded enqueue). All reaction happens single-threaded on the bot's own tick. It never reads the
// packet payload — opcode-as-signal only (handoff § 0).
class mod_playerbots_server_script : public ServerScript
{
public:
    mod_playerbots_server_script() : ServerScript("mod_playerbots_server_script") { }

    void OnPacketSend(WorldSession* session, WorldPacket& packet) override
    {
        // Cheapest bails first. The master toggle lets an operator disable the whole layer without
        // rebuilding; when off, bots fall back to their per-tick polls (we don't break things).
        if (!Playerbots::IsEnabled() || !Playerbots::GetPacketObservationEnabled())
            return;

        if (!session || !session->IsBotSession())
            return;

        Player* bot = session->GetPlayer();
        if (!bot)
            return;

        // Same AI lookup the bot's own tick uses (mod_playerbots_player_script::OnPlayerAfterUpdate).
        if (BotPlayerbotAI* ai = GET_PLAYERBOT_AI(bot))
            ai->HandleBotOutgoingPacket(packet);
    }
};

void AddSC_mod_playerbots_server_script()
{
    new mod_playerbots_server_script();
}
