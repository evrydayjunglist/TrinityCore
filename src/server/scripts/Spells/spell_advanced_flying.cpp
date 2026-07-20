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
#include "Unit.h"
#include <cmath>

enum SkyridingLiftOffSpells
{
    SPELL_SKYRIDING_LAUNCH_BOOST = 392752 // Launch Boost — retail Lift Off cast-chain child (no SpellEffect trigger parent)
};

// Sniff-derived provisional Skyward Ascent impulse (retail 12.0.7.68453, two matched samples).
// Not EffectBasePointsF/10 — that field is 450 (same as Lift Off) but retail sends Z≈49 with |xy|≈12.25.
constexpr float SKYRIDING_SKYWARD_ASCENT_IMPULSE_XY = 12.25f;
constexpr float SKYRIDING_SKYWARD_ASCENT_IMPULSE_Z = 49.0f;

// Sniff-derived provisional Surge Forward impulse (retail 12.0.7.68453 sample |v|≈18.9).
// EFFECT_0 is SPELL_EFFECT_DUMMY with EffectBasePointsF=0 — do not derive magnitude from /10.
constexpr float SKYRIDING_SURGE_FORWARD_IMPULSE_XY = 18.3f;
constexpr float SKYRIDING_SURGE_FORWARD_IMPULSE_Z = -4.71f;

// Sniff-derived provisional Whirling Surge impulse (retail 12.0.7.68453 sample |v|=60).
// No SPELL_EFFECT_DUMMY row — cast-time OnCast hook; leave APPLY_AURA area-trigger misc alone.
constexpr float SKYRIDING_WHIRLING_SURGE_IMPULSE_XY = 60.0f;
constexpr float SKYRIDING_WHIRLING_SURGE_IMPULSE_Z = 1.95f;

// 374763 - Lift Off
class spell_skyriding_lift_off : public SpellScript
{
    PrepareSpellScript(spell_skyriding_lift_off);

    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_SKYRIDING_LAUNCH_BOOST });
    }

    void HandleLaunch(SpellEffIndex /*effIndex*/)
    {
        Unit* caster = GetCaster();
        if (!caster)
            return;

        // SpellEffect base points are stored in tenths for this impulse: 450 produces the
        // (0, 0, 45) SMSG_MOVE_ADD_IMPULSE observed on retail 12.0.7.68453.
        caster->AddMoveImpulse(Position(0.0f, 0.0f, GetEffectValue() / 10.0f));

        // Retail applies Launch Boost in the same Lift Off chain (OriginalCastID → 374763).
        // No SpellEffect.EffectTriggerSpell parent exists in DB2; CastSpell(..., GetSpell())
        // preserves triggered cast linkage the way FORCE_CAST / script children do elsewhere.
        caster->CastSpell(caster, SPELL_SKYRIDING_LAUNCH_BOOST, GetSpell());
    }

    void Register() override
    {
        OnEffectHitTarget += SpellEffectFn(spell_skyriding_lift_off::HandleLaunch, EFFECT_0, SPELL_EFFECT_DUMMY);
    }
};

// 372610 - Skyward Ascent
class spell_skyriding_skyward_ascent : public SpellScript
{
    PrepareSpellScript(spell_skyriding_skyward_ascent);

    void HandleAscent(SpellEffIndex /*effIndex*/)
    {
        Unit* caster = GetCaster();
        if (!caster)
            return;

        // Facing-relative SMSG_MOVE_ADD_IMPULSE. DB2 EFFECT_0 is SPELL_EFFECT_DUMMY
        // (EffectBasePointsF=450); retail wire is not pure-up (0,0,45) — see provisional constants.
        float const o = caster->GetOrientation();
        float const xy = SKYRIDING_SKYWARD_ASCENT_IMPULSE_XY;
        caster->AddMoveImpulse(Position(std::cos(o) * xy, std::sin(o) * xy, SKYRIDING_SKYWARD_ASCENT_IMPULSE_Z));
    }

    void Register() override
    {
        OnEffectHitTarget += SpellEffectFn(spell_skyriding_skyward_ascent::HandleAscent, EFFECT_0, SPELL_EFFECT_DUMMY);
    }
};

// 372608 - Surge Forward
class spell_skyriding_surge_forward : public SpellScript
{
    PrepareSpellScript(spell_skyriding_surge_forward);

    void HandleSurge(SpellEffIndex /*effIndex*/)
    {
        Unit* caster = GetCaster();
        if (!caster)
            return;

        // Facing-relative SMSG_MOVE_ADD_IMPULSE. DB2 EFFECT_0 base points are 0; magnitudes
        // are provisional from one retail 12.0.7.68453 sample (|xy|≈18.3, Z≈-4.71 → |v|≈18.9).
        float const o = caster->GetOrientation();
        float const xy = SKYRIDING_SURGE_FORWARD_IMPULSE_XY;
        caster->AddMoveImpulse(Position(std::cos(o) * xy, std::sin(o) * xy, SKYRIDING_SURGE_FORWARD_IMPULSE_Z));
    }

    void Register() override
    {
        OnEffectHitTarget += SpellEffectFn(spell_skyriding_surge_forward::HandleSurge, EFFECT_0, SPELL_EFFECT_DUMMY);
    }
};

// 361584 - Whirling Surge
class spell_skyriding_whirling_surge : public SpellScript
{
    PrepareSpellScript(spell_skyriding_whirling_surge);

    void HandleWhirl()
    {
        Unit* caster = GetCaster();
        if (!caster)
            return;

        // Facing-relative SMSG_MOVE_ADD_IMPULSE. DB2 has APPLY_AURA rows only (EFFECT_0 aura
        // DUMMY, EFFECT_1 aura 395 misc 26554) — no SPELL_EFFECT_DUMMY to hook. OnCast matches
        // retail prepare/cast impulse timing without inventing a fake effect index.
        // Provisional from one retail 12.0.7.68453 sample (|xy|=60, Z≈1.95).
        float const o = caster->GetOrientation();
        float const xy = SKYRIDING_WHIRLING_SURGE_IMPULSE_XY;
        caster->AddMoveImpulse(Position(std::cos(o) * xy, std::sin(o) * xy, SKYRIDING_WHIRLING_SURGE_IMPULSE_Z));
    }

    void Register() override
    {
        OnCast += SpellCastFn(spell_skyriding_whirling_surge::HandleWhirl);
    }
};

void AddSC_advanced_flying_spell_scripts()
{
    RegisterSpellScript(spell_skyriding_lift_off);
    RegisterSpellScript(spell_skyriding_skyward_ascent);
    RegisterSpellScript(spell_skyriding_surge_forward);
    RegisterSpellScript(spell_skyriding_whirling_surge);
}
