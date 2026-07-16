/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "ScriptMgr.h"
#include "SpellScript.h"

// 374763 - Lift Off
class spell_skyriding_lift_off : public SpellScript
{
    PrepareSpellScript(spell_skyriding_lift_off);

    void HandleLaunch(SpellEffIndex /*effIndex*/)
    {
        // SpellEffect base points are stored in tenths for this impulse: 450 produces the
        // (0, 0, 45) SMSG_MOVE_ADD_IMPULSE observed on retail 12.0.7.68453.
        GetCaster()->AddMoveImpulse(Position(0.0f, 0.0f, GetEffectValue() / 10.0f));
    }

    void Register() override
    {
        OnEffectHitTarget += SpellEffectFn(spell_skyriding_lift_off::HandleLaunch, EFFECT_0, SPELL_EFFECT_DUMMY);
    }
};

void AddSC_advanced_flying_spell_scripts()
{
    RegisterSpellScript(spell_skyriding_lift_off);
}
