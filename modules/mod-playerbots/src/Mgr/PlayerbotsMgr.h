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

#ifndef TRINITY_PLAYERBOTS_MGR_H
#define TRINITY_PLAYERBOTS_MGR_H

#include "ObjectGuid.h"
#include <memory>
#include <unordered_map>

class BotPlayerbotAI;
class Player;
class PlayerbotMgr;

// AC reference: PlayerbotMgr.h — PlayerbotsMgr registry (_playerbotsAIMap + _playerbotsMgrMap).
class PlayerbotsMgr
{
public:
    static PlayerbotsMgr* instance();

    void AddPlayerbotData(Player* player, bool isBotAI);
    void RemovePlayerbotData(Player* player, bool isBotAI);

    BotPlayerbotAI* GetPlayerbotAI(Player* player) const;
    PlayerbotMgr* GetPlayerbotMgr(Player* player) const;

private:
    PlayerbotsMgr() = default;

    std::unordered_map<ObjectGuid, std::unique_ptr<BotPlayerbotAI>> _playerbotsAIMap;
    std::unordered_map<ObjectGuid, std::unique_ptr<PlayerbotMgr>> _playerbotsMgrMap;
};

#define sPlayerbotsMgr PlayerbotsMgr::instance()

#endif
