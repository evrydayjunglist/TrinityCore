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

#ifndef TRINITY_PLAYERBOT_AI_BASE_H
#define TRINITY_PLAYERBOT_AI_BASE_H

#include "Define.h"

class Player;

// Minimal AC-shaped tick shell (Gate 5). AC reference: Bot/Engine/PlayerbotAIBase.h
class PlayerbotAIBase
{
public:
    explicit PlayerbotAIBase(Player* bot);

    bool CanUpdateAI() const;
    void UpdateAI(uint32 diff);

    Player* GetBot() const { return _bot; }

protected:
    virtual void UpdateAIInternal(uint32 /*diff*/) { }

    void SetNextCheckDelay(uint32 delay);

private:
    Player* _bot;
    uint32 _nextAICheckDelay = 0;

    static constexpr uint32 MIN_AI_UPDATE_DELAY_MS = 100;
};

#endif
