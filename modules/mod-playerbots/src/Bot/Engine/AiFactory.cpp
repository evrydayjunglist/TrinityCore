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

#include "AiFactory.h"
#include "AiObjectContext.h"
#include "Bot/Action/AttackAction.h"
#include "Bot/Action/FollowAction.h"
#include "Bot/Action/GrindAction.h"
#include "Bot/Action/QuestGiverAction.h"
#include "Bot/Action/WanderAction.h"
#include "Bot/Strategy/CombatStrategy.h"
#include "Bot/Strategy/FollowMasterStrategy.h"
#include "Bot/Strategy/NewRpgStrategy.h"
#include "Bot/Strategy/PassiveStrategy.h"
#include "BotPlayerbotAI.h"
#include "DB2Stores.h"
#include "Log.h"
#include "Player.h"
#include "PlayerbotsConfig.h"

std::unique_ptr<AiObjectContext> AiFactory::CreateContext(BotPlayerbotAI* botAI, Player* player)
{
    auto context = std::make_unique<AiObjectContext>(botAI);
    context->RegisterStrategy("passive", std::make_unique<PassiveStrategy>(botAI));
    context->RegisterStrategy("follow", std::make_unique<FollowMasterStrategy>(botAI));
    context->RegisterStrategy("attack", std::make_unique<CombatStrategy>(botAI));
    context->RegisterStrategy("newrpg", std::make_unique<NewRpgStrategy>(botAI));
    context->RegisterAction("follow", std::make_unique<FollowAction>(botAI));
    context->RegisterAction("attack my target", std::make_unique<AttackMyTargetAction>(botAI));
    context->RegisterAction("wander", std::make_unique<WanderAction>(botAI));
    context->RegisterAction("grind", std::make_unique<GrindAction>(botAI));
    context->RegisterAction("quest giver", std::make_unique<QuestGiverAction>(botAI));

    if (Playerbots::GetLogLevel() >= 1 && player)
    {
        uint8 const playerClass = player->GetClass();
        ChrSpecialization spec = player->GetPrimarySpecialization();
        if (ChrSpecializationEntry const* specEntry = player->GetPrimarySpecializationEntry())
        {
            TC_LOG_DEBUG("playerbots", "AiFactory context bot={} class={} spec={} ({})",
                player->GetName(), playerClass, uint32(spec), specEntry->Name[LOCALE_enUS]);
        }
        else if (ChrSpecializationEntry const* defaultSpec = sDB2Manager.GetDefaultChrSpecializationForClass(playerClass))
        {
            TC_LOG_DEBUG("playerbots", "AiFactory context bot={} class={} defaultSpec={} ({})",
                player->GetName(), playerClass, defaultSpec->ID, defaultSpec->Name[LOCALE_enUS]);
        }
        else
        {
            TC_LOG_DEBUG("playerbots", "AiFactory context bot={} class={} spec=unknown",
                player->GetName(), playerClass);
        }
    }

    return context;
}
