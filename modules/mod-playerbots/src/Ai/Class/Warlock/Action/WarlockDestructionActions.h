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

#ifndef TRINITY_PLAYERBOT_WARLOCK_DESTRUCTION_ACTIONS_H
#define TRINITY_PLAYERBOT_WARLOCK_DESTRUCTION_ACTIONS_H

#include "Ai/Class/Warlock/WarlockSpellIds.h"
#include "Bot/Action/CastSpellAction.h"
#include "SharedDefines.h"
#include "Unit.h"

class BotPlayerbotAI;
class Player;

class CastImmolateAction : public CastSpellAction
{
public:
    explicit CastImmolateAction(BotPlayerbotAI* botAI)
        : CastSpellAction(botAI, "immolate", Playerbots::Warlock::Destruction::SPELL_IMMOLATE) { }

protected:
    bool IsUsefulExtra(Player* /*bot*/, Unit* target) const override
    {
        // Maintain the DoT aura (157736), not cast id 348 (direct + dummy only).
        return target && !target->HasAura(Playerbots::Warlock::Destruction::SPELL_IMMOLATE_DOT);
    }
};

class CastIncinerateAction : public CastSpellAction
{
public:
    explicit CastIncinerateAction(BotPlayerbotAI* botAI)
        : CastSpellAction(botAI, "incinerate", Playerbots::Warlock::Destruction::SPELL_INCINERATE) { }
};

class CastConflagrateAction : public CastSpellAction
{
public:
    explicit CastConflagrateAction(BotPlayerbotAI* botAI)
        : CastSpellAction(botAI, "conflagrate", Playerbots::Warlock::Destruction::SPELL_CONFLAGRATE) { }
};

class CastChaosBoltAction : public CastSpellAction
{
public:
    explicit CastChaosBoltAction(BotPlayerbotAI* botAI)
        : CastSpellAction(botAI, "chaos bolt", Playerbots::Warlock::Destruction::SPELL_CHAOS_BOLT) { }

protected:
    bool IsUsefulExtra(Player* bot, Unit* /*target*/) const override
    {
        return bot && bot->GetPower(POWER_SOUL_SHARDS) >=
            int32(Playerbots::Warlock::Destruction::CHAOS_BOLT_MIN_SOUL_SHARDS);
    }
};

class CastUnendingResolveAction : public CastSelfBuffSpellAction
{
public:
    explicit CastUnendingResolveAction(BotPlayerbotAI* botAI)
        : CastSelfBuffSpellAction(botAI, "unending resolve",
            Playerbots::Warlock::Destruction::SPELL_UNENDING_RESOLVE) { }

protected:
    bool IsUsefulExtra(Player* bot, Unit* target) const override
    {
        if (!CastSelfBuffSpellAction::IsUsefulExtra(bot, target))
            return false;

        // Defensive CD — only when hurt in combat.
        return bot->IsInCombat() && bot->GetHealthPct() < 50.0f;
    }
};

#endif
