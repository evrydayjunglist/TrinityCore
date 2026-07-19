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

#include "CombatTriggers.h"
#include "Bot/Action/CombatTargetSelect.h"
#include "BotPlayerbotAI.h"
#include "Player.h"
#include "Playerbots.h"
#include "PlayerbotsConfig.h"

bool HasCombatTargetTrigger::IsActive()
{
    if (!_botAI)
        return false;

    return HasValidCombatTarget(_botAI, _botAI->GetBot());
}

bool HasAttackersTrigger::IsActive()
{
    if (!_botAI)
        return false;

    AiObjectContext* context = _botAI->GetAiObjectContext();
    if (!context)
        return false;

    return AI_VALUE(uint32, "attacker count") > 0;
}

bool FleeHealthTrigger::IsActive()
{
    if (!_botAI || !Playerbots::GetCombatFleeEnabled())
        return false;

    Player* bot = _botAI->GetBot();
    if (!bot || !bot->IsAlive() || !bot->IsInWorld())
        return false;

    AiObjectContext* context = _botAI->GetAiObjectContext();
    if (!context)
        return false;

    uint32 const attackers = AI_VALUE(uint32, "attacker count");
    if (!bot->IsInCombat() && attackers == 0)
    {
        if (AI_VALUE(bool, "is fleeing"))
            SET_AI_VALUE(bool, "is fleeing", false);
        return false;
    }

    uint8 const health = AI_VALUE(uint8, "health");
    bool const fleeing = AI_VALUE(bool, "is fleeing");
    uint8 const enterPct = Playerbots::GetCombatFleeHealthPct();
    uint8 const exitPct = Playerbots::GetCombatFleeHealthExitPct();

    if (fleeing)
        return health <= exitPct;

    return health <= enterPct;
}
