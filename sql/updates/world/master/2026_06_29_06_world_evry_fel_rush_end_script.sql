-- Fel Rush dash end helper (346123) — dest snap at caster position (retail momentum end)
DELETE FROM `spell_script_names` WHERE `ScriptName`='spell_dh_fel_rush_end';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(346123, 'spell_dh_fel_rush_end');
