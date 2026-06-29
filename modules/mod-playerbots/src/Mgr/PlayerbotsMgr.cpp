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

#include "PlayerbotsMgr.h"
#include "BotPlayerbotAI.h"
#include "Player.h"
#include "PlayerbotMgr.h"
#include "PlayerbotsConfig.h"

PlayerbotsMgr* PlayerbotsMgr::instance()
{
    static PlayerbotsMgr instance;
    return &instance;
}

void PlayerbotsMgr::AddPlayerbotData(Player* player, bool isBotAI)
{
    if (!player || !Playerbots::IsEnabled())
        return;

    if (isBotAI)
    {
        _playerbotsAIMap[player->GetGUID()] = std::make_unique<BotPlayerbotAI>(player);
        return;
    }

    _playerbotsMgrMap[player->GetGUID()] = std::make_unique<PlayerbotMgr>(player);
}

void PlayerbotsMgr::RemovePlayerbotData(Player* player, bool isBotAI)
{
    if (!player)
        return;

    if (isBotAI)
        _playerbotsAIMap.erase(player->GetGUID());
    else
        _playerbotsMgrMap.erase(player->GetGUID());
}

BotPlayerbotAI* PlayerbotsMgr::GetPlayerbotAI(Player* player) const
{
    if (!player || !Playerbots::IsEnabled())
        return nullptr;

    auto itr = _playerbotsAIMap.find(player->GetGUID());
    return itr != _playerbotsAIMap.end() ? itr->second.get() : nullptr;
}

PlayerbotMgr* PlayerbotsMgr::GetPlayerbotMgr(Player* player) const
{
    if (!player || !Playerbots::IsEnabled())
        return nullptr;

    auto itr = _playerbotsMgrMap.find(player->GetGUID());
    return itr != _playerbotsMgrMap.end() ? itr->second.get() : nullptr;
}
