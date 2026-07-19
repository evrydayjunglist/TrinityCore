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

#ifndef TRINITY_PLAYERBOT_HUNTER_BEAST_MASTERY_ACTIONS_H
#define TRINITY_PLAYERBOT_HUNTER_BEAST_MASTERY_ACTIONS_H

#include "Ai/Class/Hunter/HunterSpellIds.h"
#include "Bot/Action/CastSpellAction.h"

class BotPlayerbotAI;
class Player;
class Unit;

class CastKillCommandAction : public CastSpellAction
{
public:
    explicit CastKillCommandAction(BotPlayerbotAI* botAI)
        : CastSpellAction(botAI, "kill command", Playerbots::Hunter::BeastMastery::SPELL_KILL_COMMAND) { }
};

class CastBarbedShotAction : public CastSpellAction
{
public:
    explicit CastBarbedShotAction(BotPlayerbotAI* botAI)
        : CastSpellAction(botAI, "barbed shot", Playerbots::Hunter::BeastMastery::SPELL_BARBED_SHOT) { }
};

class CastCobraShotAction : public CastSpellAction
{
public:
    explicit CastCobraShotAction(BotPlayerbotAI* botAI)
        : CastSpellAction(botAI, "cobra shot", Playerbots::Hunter::BeastMastery::SPELL_COBRA_SHOT) { }
};

class CastExhilarationAction : public CastSelfBuffSpellAction
{
public:
    explicit CastExhilarationAction(BotPlayerbotAI* botAI)
        : CastSelfBuffSpellAction(botAI, "exhilaration",
            Playerbots::Hunter::BeastMastery::SPELL_EXHILARATION) { }

protected:
    bool IsUsefulExtra(Player* bot, Unit* target) const override
    {
        if (!CastSelfBuffSpellAction::IsUsefulExtra(bot, target))
            return false;

        // Heal when hurt — skip healthy OOC spam.
        return bot->GetHealthPct() < 70.0f;
    }
};

#endif
