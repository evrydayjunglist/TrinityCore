-- SPELL_STASIS_3 (366636): quest popup with Kodethi as giver (core EffectQuestStart uses player GUID and client drops DisplayPopup).
DELETE FROM `spell_script_names` WHERE `spell_id` = 366636 AND `ScriptName` = 'spell_dracthyr_stasis_3';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(366636, 'spell_dracthyr_stasis_3');
