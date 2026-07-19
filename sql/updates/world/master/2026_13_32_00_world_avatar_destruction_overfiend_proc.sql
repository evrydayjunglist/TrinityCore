-- Avatar of Destruction (1245089): Overfiend trigger 434587
-- DB2 SpellAuraOptions: ProcTypeMask = 0;4 (PROC_FLAG_2_CAST_SUCCESSFUL), ProcChance 101,
-- but SpellEffect SpellClassMask is empty - generated spell_proc therefore matches every cast.
-- Restrict to Soul Fire (SpellFamily Warlock, class mask 0;131072;2;0). Dimensional Rift chance path NYI.
DELETE FROM `spell_proc` WHERE `SpellId` IN (1245089);
INSERT INTO `spell_proc` (`SpellId`,`SchoolMask`,`SpellFamilyName`,`SpellFamilyMask0`,`SpellFamilyMask1`,`SpellFamilyMask2`,`SpellFamilyMask3`,`ProcFlags`,`ProcFlags2`,`SpellTypeMask`,`SpellPhaseMask`,`HitMask`,`AttributesMask`,`DisableEffectsMask`,`ProcsPerMinute`,`Chance`,`Cooldown`,`Charges`) VALUES
(1245089,0x00,5,0x00000000,0x00020000,0x00000002,0x00000000,0x0,0x4,0x0,0x0,0x0,0x0,0x0,0,0,0,0); -- Avatar of Destruction
