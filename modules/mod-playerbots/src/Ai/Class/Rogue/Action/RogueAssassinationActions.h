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

#ifndef TRINITY_PLAYERBOT_ROGUE_ASSASSINATION_ACTIONS_H
#define TRINITY_PLAYERBOT_ROGUE_ASSASSINATION_ACTIONS_H

#include "Ai/Class/Rogue/RogueSpellIds.h"
#include "Bot/Action/CastSpellAction.h"
#include "SharedDefines.h"
#include "Unit.h"

class BotPlayerbotAI;
class Player;

// Thin named wrappers — Gates 15–17 copy this pattern per class/spec.

class CastGarroteAction : public CastSpellAction
{
public:
    explicit CastGarroteAction(BotPlayerbotAI* botAI)
        : CastSpellAction(botAI, "garrote", Playerbots::Rogue::Assassination::SPELL_GARROTE) { }

protected:
    bool IsUsefulExtra(Player* /*bot*/, Unit* target) const override
    {
        // Opener: apply bleed when missing on current target.
        return target && !target->HasAura(_spellId);
    }
};

class CastMutilateAction : public CastSpellAction
{
public:
    explicit CastMutilateAction(BotPlayerbotAI* botAI)
        : CastSpellAction(botAI, "mutilate", Playerbots::Rogue::Assassination::SPELL_MUTILATE) { }
};

class CastEnvenomAction : public CastSpellAction
{
public:
    explicit CastEnvenomAction(BotPlayerbotAI* botAI)
        : CastSpellAction(botAI, "envenom", Playerbots::Rogue::Assassination::SPELL_ENVENOM) { }

protected:
    bool IsUsefulExtra(Player* bot, Unit* /*target*/) const override
    {
        return bot && bot->GetPower(POWER_COMBO_POINTS) >=
            int32(Playerbots::Rogue::Assassination::ENVENOM_MIN_COMBO_POINTS);
    }
};

class CastCrimsonVialAction : public CastSelfBuffSpellAction
{
public:
    explicit CastCrimsonVialAction(BotPlayerbotAI* botAI)
        : CastSelfBuffSpellAction(botAI, "crimson vial",
            Playerbots::Rogue::Assassination::SPELL_CRIMSON_VIAL) { }

protected:
    bool IsUsefulExtra(Player* bot, Unit* target) const override
    {
        if (!CastSelfBuffSpellAction::IsUsefulExtra(bot, target))
            return false;

        // Out of combat: show the self-buff. In combat: only when hurt (don't starve fillers).
        return !bot->IsInCombat() || bot->GetHealthPct() < 70.0f;
    }
};

#endif
