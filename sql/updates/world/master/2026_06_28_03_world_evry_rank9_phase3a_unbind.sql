-- Rank 9 Phase 3A — SQL unbind when Midnight SpellEffect.db2 (12.0.7.67808) proves script hooks obsolete
-- 5246: no area-select effects (all single-target auras); area filter script obsolete
DELETE FROM `spell_script_names` WHERE `spell_id`=5246 AND `ScriptName`='spell_warr_intimidating_shout';
-- 123456: single E0 apply-aura only; destination offset hooks obsolete
DELETE FROM `spell_script_names` WHERE `spell_id`=123456 AND `ScriptName`='spell_halion_summon_exit_portals';
-- 343294: no SPELL_AURA_PERIODIC_DUMMY on parent (tick via triggered spell); keep 469180 reaper_of_souls registration
DELETE FROM `spell_script_names` WHERE `spell_id`=343294 AND `ScriptName`='spell_dk_soul_reaper';
