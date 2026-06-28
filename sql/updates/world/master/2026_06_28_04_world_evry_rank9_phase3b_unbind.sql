-- Rank 9 Phase 3B — SQL unbind when Midnight SpellEffect.db2 (12.0.7.67808) proves script hooks obsolete
-- 453282: E0 ADD_PCT_MODIFIER only; periodic dummy tick script obsolete
DELETE FROM `spell_script_names` WHERE `spell_id`=453282 AND `ScriptName`='spell_mage_flame_accelerant';
-- 157982: direct heal (effect 10) only; aura calc-healing hook belongs on periodic hosts (271466, 363534)
DELETE FROM `spell_script_names` WHERE `spell_id`=157982 AND `ScriptName`='spell_gen_major_healing_cooldown_modifier_aura';
-- 81690: E0 PERIODIC_DAMAGE aura only; script-effect taunt hook obsolete
DELETE FROM `spell_script_names` WHERE `spell_id`=81690 AND `ScriptName`='spell_gen_random_aggro_taunt';
