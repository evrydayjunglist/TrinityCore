-- Fel Rush momentum (197923 air dash) — child spell gaps + dead trigger removal
-- Evidence: EvryDb2Export @ 12.0.7.67808; retail sniff 2026-06-29; tc-64864_parsed new.txt
-- 197707 absent from client SpellName; 199737/346123 fail CAST_FAILED (32) on wire when DB2-triggered

-- Stop triggering removed spell 197707 (197923 E6)
UPDATE `spell_effect` SET `EffectTriggerSpell`=0 WHERE `ID`=290865;

-- Script applies 199737 / 346123 with TRIGGERED_FULL_MASK; silence failing DB2 triggers (E8/E10)
UPDATE `spell_effect` SET `EffectTriggerSpell`=0 WHERE `ID` IN (296647, 872448);

DELETE FROM `spell_name` WHERE `VerifiedBuild`>0 AND `ID` IN (199737, 346123);
INSERT INTO `spell_name` (`ID`, `Name`, `VerifiedBuild`) VALUES
(199737, 'Fel Rush', 67808),
(346123, 'Fel Rush', 67808);

DELETE FROM `spell_misc` WHERE `VerifiedBuild`>0 AND `ID` IN (173751, 443761);
INSERT INTO `spell_misc` (`ID`, `Attributes1`, `Attributes2`, `Attributes3`, `Attributes4`, `Attributes5`, `Attributes6`, `Attributes7`, `Attributes8`, `Attributes9`, `Attributes10`, `Attributes11`, `Attributes12`, `Attributes13`, `Attributes14`, `Attributes15`, `Attributes16`, `Attributes17`, `DifficultyID`, `CastingTimeIndex`, `DurationIndex`, `PvPDurationIndex`, `RangeIndex`, `SchoolMask`, `Speed`, `LaunchDelay`, `MinDuration`, `SpellIconFileDataID`, `ActiveIconFileDataID`, `ContentTuningID`, `ShowFutureSpellPlayerConditionID`, `SpellVisualScript`, `ActiveSpellVisualScript`, `SpellID`, `VerifiedBuild`) VALUES
(173751, 8388864, 268435456, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 28, 0, 1, 1, 0, 0, 0, 135561, 0, 0, 0, 0, 0, 199737, 67808),
(443761, -2139029360, 268567584, 262144, 4, 32, 0, 1025, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 0, 0, 0, 1247261, 0, 0, 0, 0, 0, 346123, 67808);

DELETE FROM `spell_effect` WHERE `VerifiedBuild`>0 AND `ID` IN (293934, 872445);
INSERT INTO `spell_effect` (`ID`, `EffectAura`, `DifficultyID`, `EffectIndex`, `Effect`, `EffectAmplitude`, `EffectAttributes`, `EffectAuraPeriod`, `EffectBonusCoefficient`, `EffectChainAmplitude`, `EffectChainTargets`, `EffectItemType`, `EffectMechanic`, `EffectPointsPerResource`, `EffectPosFacing`, `EffectRealPointsPerLevel`, `EffectTriggerSpell`, `BonusCoefficientFromAP`, `PvpMultiplier`, `Coefficient`, `Variance`, `ResourceCoefficient`, `GroupSizeBasePointsCoefficient`, `EffectBasePoints`, `ScalingClass`, `TargetNodeGraph`, `EffectMiscValue1`, `EffectMiscValue2`, `EffectRadiusIndex1`, `EffectRadiusIndex2`, `EffectSpellClassMask1`, `EffectSpellClassMask2`, `EffectSpellClassMask3`, `EffectSpellClassMask4`, `ImplicitTarget1`, `ImplicitTarget2`, `SpellID`, `VerifiedBuild`) VALUES
(293934, 312, 0, 0, 6, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 496, 0, 0, 0, 0, 0, 0, 0, 1, 0, 199737, 67808),
(872445, 0, 0, 0, 3, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 18, 0, 346123, 67808);
