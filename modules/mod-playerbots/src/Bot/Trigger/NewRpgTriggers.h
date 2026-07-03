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

#ifndef TRINITY_PLAYERBOT_NEW_RPG_TRIGGERS_H
#define TRINITY_PLAYERBOT_NEW_RPG_TRIGGERS_H

#include "Bot/Rpg/NewRpgInfo.h"
#include "Trigger.h"

class BotPlayerbotAI;

// Gate 10b — status-gating trigger. AC reference: mod-playerbots-master/src/Ai/World/Rpg/
// Trigger/NewRpgTriggers.h NewRpgStatusTrigger — active exactly while the bot's RPG state
// machine sits in the given status, so exactly one per-status action runs per tick.
class NewRpgStatusTrigger : public Trigger
{
public:
    NewRpgStatusTrigger(BotPlayerbotAI* botAI, NewRpgStatus status)
        : Trigger(botAI, "new rpg status"), _status(status) { }

    bool IsActive() override;

private:
    NewRpgStatus _status;
};

#endif
