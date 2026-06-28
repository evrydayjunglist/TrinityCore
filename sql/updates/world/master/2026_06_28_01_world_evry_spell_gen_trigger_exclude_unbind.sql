-- Rank 3: spell_gen_trigger_exclude_* — unbind spells whose Exclude*AuraSpell points to
-- ids removed from Spell.db2 on build 12.0.7.67808 (SpellAuraRestrictions.db2 / wago.tools).
-- Field=0 cases are handled in spell_generic.cpp (R3-A); these rows had non-zero stale refs.
DELETE FROM `spell_script_names` WHERE `ScriptName` = 'spell_gen_trigger_exclude_caster_aura_spell' AND `spell_id` IN (
    172,    -- ExcludeCasterAuraSpell 103958 (missing)
    1454,   -- 159669
    131632, -- 114168
    143395, -- 114168
    178338, -- 115070
    194834, -- 90259
    212284, -- 159669
    213398, -- 74434
    213831, -- 117050
    219779  -- 90259
);
DELETE FROM `spell_script_names` WHERE `ScriptName` = 'spell_gen_trigger_exclude_target_aura_spell' AND `spell_id` IN (
    31790,  -- ExcludeTargetAuraSpell 86315 (missing)
    100950  -- 86315
);
