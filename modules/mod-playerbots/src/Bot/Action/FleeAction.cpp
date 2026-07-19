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

#include "FleeAction.h"
#include "AiObjectContext.h"
#include "Bot/Engine/Value.h"
#include "BotPlayerbotAI.h"
#include "Mgr/Move/FleeManager.h"
#include "MotionMaster.h"
#include "Player.h"
#include "Playerbots.h"
#include "PlayerbotsConfig.h"
#include "SafeMovement.h"

namespace
{
bool ShouldFleeNow(BotPlayerbotAI* botAI, Player* bot)
{
    if (!botAI || !bot || !Playerbots::GetCombatFleeEnabled())
        return false;

    if (!bot->IsAlive() || !bot->IsInWorld())
        return false;

    AiObjectContext* context = botAI->GetAiObjectContext();
    if (!context)
        return false;

    uint32 const attackers = context->GetValue<uint32>("attacker count")->Get();
    if (!bot->IsInCombat() && attackers == 0)
    {
        if (context->GetValue<bool>("is fleeing")->Get())
            context->GetValue<bool>("is fleeing")->Set(false);
        return false;
    }

    uint8 const health = context->GetValue<uint8>("health")->Get();
    bool const fleeing = context->GetValue<bool>("is fleeing")->Get();
    uint8 const enterPct = Playerbots::GetCombatFleeHealthPct();
    uint8 const exitPct = Playerbots::GetCombatFleeHealthExitPct();

    if (fleeing)
        return health <= exitPct;

    return health <= enterPct;
}
}

bool FleeAction::IsUseful()
{
    return ShouldFleeNow(_botAI, GetBot());
}

bool FleeAction::Execute(Event /*event*/)
{
    Player* bot = GetBot();
    if (!bot || !ShouldFleeNow(_botAI, bot))
        return false;

    FleeManager flee(bot, Playerbots::GetCombatFleeDistance());
    float x = 0.0f, y = 0.0f, z = 0.0f;
    if (!flee.CalculateDestination(x, y, z))
        return false;

    // Clear chase/follow so SafeMovement's MovePoint owns the slot.
    bot->AttackStop();
    bot->GetMotionMaster()->Clear();

    if (!TryMoveToValidatedPoint(bot, x, y, z))
        return false;

    SET_AI_VALUE(bool, "is fleeing", true);
    return true;
}
