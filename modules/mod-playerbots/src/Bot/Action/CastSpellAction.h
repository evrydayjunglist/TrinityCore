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

#ifndef TRINITY_PLAYERBOT_CAST_SPELL_ACTION_H
#define TRINITY_PLAYERBOT_CAST_SPELL_ACTION_H

#include "Action.h"
#include "Define.h"

class BotPlayerbotAI;
class Player;
class SpellInfo;
class Unit;

// Gate 14 — AC-shaped CastSpellAction; Midnight spell id + TC WorldObject::CastSpell.
// Never QueuePacket / CMSG_CAST_SPELL. IsUseful fails cleanly on GCD / CD / range / silence /
// unknown / unlearned spells.
class CastSpellAction : public Action
{
public:
    CastSpellAction(BotPlayerbotAI* botAI, std::string name, uint32 spellId);

    bool Execute(Event event) override;
    bool IsUseful() override;
    bool IsPossible() override;

    uint32 GetSpellId() const { return _spellId; }

protected:
    // Combat casts → AI_VALUE("current target"); buffs override to self.
    virtual Unit* GetSpellTarget() const;

    // Extra usefulness after shared castability (aura missing, combo points, …).
    virtual bool IsUsefulExtra(Player* /*bot*/, Unit* /*target*/) const { return true; }

    // When true (default), face / Attack / Gate 12 combat movement before cast.
    virtual bool ShouldPrepareCombatSwing() const { return true; }

    bool CanCastOn(Player* bot, Unit* target, SpellInfo const* spellInfo) const;

    uint32 _spellId;
};

// Self-target buff/heal twin — skips combat swing prep; default Extra = aura not present.
class CastSelfBuffSpellAction : public CastSpellAction
{
public:
    CastSelfBuffSpellAction(BotPlayerbotAI* botAI, std::string name, uint32 spellId);

protected:
    Unit* GetSpellTarget() const override;
    bool ShouldPrepareCombatSwing() const override { return false; }
    bool IsUsefulExtra(Player* bot, Unit* target) const override;
};

#endif
