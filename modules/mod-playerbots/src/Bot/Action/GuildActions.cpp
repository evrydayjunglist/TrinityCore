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

#include "GuildActions.h"
#include "BotPlayerbotAI.h"
#include "GuildPackets.h"
#include "Log.h"
#include "Opcodes.h"
#include "Player.h"
#include "PlayerbotsConfig.h"
#include "WorldPacket.h"
#include "WorldSession.h"

namespace
{
bool HasMasterGuildInvite(BotPlayerbotAI* botAI, Player*& bot, Player*& master)
{
    if (!botAI)
        return false;

    bot = botAI->GetBot();
    master = botAI->GetMaster();
    if (!bot || !master)
        return false;

    ObjectGuid::LowType const invited = bot->GetGuildIdInvited();
    if (!invited)
        return false;

    // Master-alt only: accept invites into the master's current guild (party-invite pattern).
    return master->GetGuildId() == invited && !bot->GetGuildId();
}
}

bool GuildAcceptAction::IsUseful()
{
    Player* bot = nullptr;
    Player* master = nullptr;
    return HasMasterGuildInvite(_botAI, bot, master);
}

bool GuildAcceptAction::Execute(Event /*event*/)
{
    Player* bot = nullptr;
    Player* master = nullptr;
    if (!HasMasterGuildInvite(_botAI, bot, master))
        return false;

    WorldPacket packet(CMSG_ACCEPT_GUILD_INVITE);
    WorldPackets::Guild::AcceptGuildInvite accept(std::move(packet));
    bot->GetSession()->HandleGuildAcceptInvite(accept);

    bool const joined = bot->GetGuildId() == master->GetGuildId() && bot->GetGuildId() != 0;

    if (Playerbots::GetLogLevel() >= 1)
        TC_LOG_DEBUG("playerbots", "GuildAcceptAction bot={} master={} joined={}",
            bot->GetName(), master->GetName(), joined ? "yes" : "no");

    return joined;
}
