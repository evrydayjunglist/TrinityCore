-- Rank 4: spell_gen_clone — unbind spells with no SPELL_EFFECT_SCRIPT_EFFECT in SpellEffect.db2 (12.0.7.67808).
-- Evidence: wago.tools SpellEffect for 49889, 50218, 51719 — DifficultyID 0 rows are Effect=6 (apply aura) only; no Effect=77.
DELETE FROM `spell_script_names` WHERE `ScriptName` = 'spell_gen_clone' AND `spell_id` IN (49889, 50218, 51719);
