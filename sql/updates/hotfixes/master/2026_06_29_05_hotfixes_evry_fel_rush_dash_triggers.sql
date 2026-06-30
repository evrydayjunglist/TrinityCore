-- Fel Rush air dash (197923) — restore retail child triggers E8/E10 (Tier 2 dash movement sync)
-- E6 stays zero (dead 197707). Requires spell_name/spell_misc rows from 2026_06_29_04.
-- Evidence: temp/db2/fel-rush-momentum/SpellEffect-197707-197923-199737-346123.csv @ 67808

UPDATE `spell_effect` SET `EffectTriggerSpell`=199737 WHERE `ID`=296647 AND `SpellID`=197923;
UPDATE `spell_effect` SET `EffectTriggerSpell`=346123 WHERE `ID`=872448 AND `SpellID`=197923;
