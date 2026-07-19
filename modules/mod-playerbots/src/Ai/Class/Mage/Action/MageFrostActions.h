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

#ifndef TRINITY_PLAYERBOT_MAGE_FROST_ACTIONS_H
#define TRINITY_PLAYERBOT_MAGE_FROST_ACTIONS_H

#include "Ai/Class/Mage/MageSpellIds.h"
#include "Bot/Action/CastSpellAction.h"

class BotPlayerbotAI;
class Player;
class Unit;

class CastFrostboltAction : public CastSpellAction
{
public:
    explicit CastFrostboltAction(BotPlayerbotAI* botAI)
        : CastSpellAction(botAI, "frostbolt", Playerbots::Mage::Frost::SPELL_FROSTBOLT) { }
};

class CastIceLanceAction : public CastSpellAction
{
public:
    explicit CastIceLanceAction(BotPlayerbotAI* botAI)
        : CastSpellAction(botAI, "ice lance", Playerbots::Mage::Frost::SPELL_ICE_LANCE) { }
};

class CastIceBarrierAction : public CastSelfBuffSpellAction
{
public:
    explicit CastIceBarrierAction(BotPlayerbotAI* botAI)
        : CastSelfBuffSpellAction(botAI, "ice barrier", Playerbots::Mage::Frost::SPELL_ICE_BARRIER) { }

protected:
    bool IsUsefulExtra(Player* bot, Unit* target) const override
    {
        if (!CastSelfBuffSpellAction::IsUsefulExtra(bot, target))
            return false;

        // OOC: keep barrier up. In combat: only when hurt (don't starve fillers).
        return !bot->IsInCombat() || bot->GetHealthPct() < 80.0f;
    }
};

#endif
