-- Rank 9 Phase 3C — SQL unbind when Midnight SpellEffect.db2 (12.0.7.67808) proves script hooks obsolete
-- 77220: E0 DUMMY only; mastery absorb hooks (E1/E2) removed on 67808
DELETE FROM `spell_script_names` WHERE `spell_id`=77220 AND `ScriptName`='spell_warl_chaotic_energies';
-- 41341: E0 DUMMY only; no SCHOOL_ABSORB for OnEffectAbsorb macro
DELETE FROM `spell_script_names` WHERE `spell_id`=41341 AND `ScriptName`='spell_illidari_council_balance_of_power';
-- 215568: Fresh Meat talent; E0 ADD_FLAT_MODIFIER only — not Bloodthirst school damage
DELETE FROM `spell_script_names` WHERE `spell_id`=215568 AND `ScriptName`='spell_warr_bloodthirst';
-- 21562: stat buff via apply auras + triggered spell; E0 dummy script-effect hook obsolete
DELETE FROM `spell_script_names` WHERE `spell_id`=21562 AND `ScriptName`='spell_pri_power_word_fortitude';
-- 34799: E0 WEAPON_PERCENT_DAMAGE only; AfterApply dummy hook obsolete
DELETE FROM `spell_script_names` WHERE `spell_id`=34799 AND `ScriptName`='spell_commander_sarannis_arcane_devastation';
-- 44436: E0 MOD_INCREASE_SPEED aura only; script-effect hook obsolete
DELETE FROM `spell_script_names` WHERE `spell_id`=44436 AND `ScriptName`='spell_hallow_end_tricky_treat';
-- 321706: E0 INSTAKILL only; area target hook obsolete
DELETE FROM `spell_script_names` WHERE `spell_id`=321706 AND `ScriptName`='spell_torghast_dimensional_blade';
-- 18813: E0 SCHOOL_DAMAGE + E1 KNOCK_BACK only; script-effect threat hook obsolete (25778 retains E2)
DELETE FROM `spell_script_names` WHERE `spell_id`=18813 AND `ScriptName`='spell_gen_knock_away_threat_reduction_25';
