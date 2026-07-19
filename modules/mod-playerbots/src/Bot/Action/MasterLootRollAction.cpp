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

#include "MasterLootRollAction.h"
#include "BotPlayerbotAI.h"
#include "Log.h"
#include "Loot.h"
#include "LootPackets.h"
#include "Opcodes.h"
#include "Player.h"
#include "PlayerbotsConfig.h"
#include "Util.h"
#include "WorldPacket.h"
#include "WorldSession.h"

bool MasterLootRollAction::IsUseful()
{
    return _botAI && _botAI->GetBot() && _botAI->GetPendingMasterLootRoll().has_value();
}

bool MasterLootRollAction::Execute(Event /*event*/)
{
    if (!_botAI)
        return false;

    std::optional<BotPlayerbotAI::PendingMasterLootRoll> const pending = _botAI->GetPendingMasterLootRoll();
    _botAI->ClearPendingMasterLootRoll();
    if (!pending)
        return false;

    Player* bot = _botAI->GetBot();
    if (!bot || !bot->GetSession())
        return false;

    // Re-check live roll before voting (Layer 2 preferred it; may have expired).
    if (!bot->GetLootRoll(pending->LootObj, pending->LootListID))
        return false;

    WorldPacket packet(CMSG_LOOT_ROLL);
    WorldPackets::Loot::LootRoll roll(std::move(packet));
    roll.LootObj = pending->LootObj;
    roll.LootListID = pending->LootListID;
    roll.RollType = AsUnderlyingType(RollVote::Pass);
    bot->GetSession()->HandleLootRoll(roll);

    if (Playerbots::GetLogLevel() >= 1)
        TC_LOG_DEBUG("playerbots", "MasterLootRollAction bot={} lootObj={} listId={} itemId={} vote=Pass",
            bot->GetName(),
            pending->LootObj.ToString(),
            uint32(pending->LootListID),
            pending->ItemID);

    return true;
}
