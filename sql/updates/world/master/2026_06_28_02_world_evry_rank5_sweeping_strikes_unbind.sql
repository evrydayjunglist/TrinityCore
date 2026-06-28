-- Rank 5-C: spell_warr_sweeping_strikes — unbind legacy hosts whose proc spells
-- 12723 / 26654 were removed from Spell.db2 on build 12.0.7.67808.
-- Retail player Sweeping Strikes is 260708 (DBC-only; not script-bound).
DELETE FROM `spell_script_names` WHERE `ScriptName` = 'spell_warr_sweeping_strikes' AND `spell_id` IN (
    12328,  -- legacy SS aura; proc ids 12723/26654 missing from Spell.db2
    18765,  -- legacy SS aura; same
    35429   -- legacy SS aura; E0=WEAPON_PERCENT_DAMAGE in DBC, script expected SPELL_AURA_DUMMY
);
