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

#ifndef TRINITY_MODULE_PLAYERBOTS_H
#define TRINITY_MODULE_PLAYERBOTS_H

#include "Bot/Engine/AiObjectContext.h"
#include "PlayerbotsMgr.h"

// AC reference: Script/Playerbots.h
#define GET_PLAYERBOT_AI(player) sPlayerbotsMgr->GetPlayerbotAI(player)
#define GET_PLAYERBOT_MGR(player) sPlayerbotsMgr->GetPlayerbotMgr(player)

// Gate 11 — AC AI_VALUE macros. Call sites must have `_botAI` in scope (Action / UntypedValue /
// Trigger via PlayerbotAIAware). Eager AiFactory registration finishes before these run, so
// GetAiObjectContext() is non-null on the bot AI tick.
#define AI_VALUE(type, name) (_botAI->GetAiObjectContext()->GetValue<type>(name)->Get())
#define AI_VALUE2(type, name, param) (_botAI->GetAiObjectContext()->GetValue<type>(name, param)->Get())

#define AI_VALUE_LAZY(type, name) (_botAI->GetAiObjectContext()->GetValue<type>(name)->LazyGet())
#define AI_VALUE2_LAZY(type, name, param) (_botAI->GetAiObjectContext()->GetValue<type>(name, param)->LazyGet())

#define SET_AI_VALUE(type, name, value) (_botAI->GetAiObjectContext()->GetValue<type>(name)->Set(value))
#define SET_AI_VALUE2(type, name, param, value) (_botAI->GetAiObjectContext()->GetValue<type>(name, param)->Set(value))
#define RESET_AI_VALUE(type, name) (_botAI->GetAiObjectContext()->GetValue<type>(name)->Reset())
#define RESET_AI_VALUE2(type, name, param) (_botAI->GetAiObjectContext()->GetValue<type>(name, param)->Reset())

#endif
