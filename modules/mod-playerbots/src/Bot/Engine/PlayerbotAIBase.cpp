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

#include "PlayerbotAIBase.h"
#include "PlayerbotsConfig.h"
#include "Player.h"
#include <algorithm>

PlayerbotAIBase::PlayerbotAIBase(Player* bot) : _bot(bot)
{
}

bool PlayerbotAIBase::CanUpdateAI() const
{
    return _nextAICheckDelay == 0;
}

void PlayerbotAIBase::UpdateAI(uint32 diff)
{
    if (_nextAICheckDelay > diff)
    {
        _nextAICheckDelay -= diff;
        return;
    }

    _nextAICheckDelay = 0;

    if (!CanUpdateAI())
        return;

    UpdateAIInternal(diff);
    SetNextCheckDelay(std::max(Playerbots::GetReactDelay(), MIN_AI_UPDATE_DELAY_MS));
}

void PlayerbotAIBase::SetNextCheckDelay(uint32 delay)
{
    _nextAICheckDelay = delay;
}
