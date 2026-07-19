-- Arathi RPE Phase 3b polish (sniff J `12-11-14` + login `23-32-20`, build 12.0.7.68453):
-- 1) Pad StandStates: Win'sa kneel, one Horde Grunt sit, one Horde Grunt dead.
-- 2) scene_template for pad PLAY_SCENE auras (true Jaina stasis cloud is scene actors).
--
-- Owner correction: 245027 outdoor Assailants are NOT the unselectable stasis cloud.
-- Stasis cloud: SceneID 3749 via spell 1248494 (SPELL_AURA_PLAY_SCENE misc 3749).
-- Ambient pad: SceneID 3692 via spell 1237116 (SPELL_AURA_PLAY_SCENE misc 3692).

-- ---------------------------------------------------------------------------
-- StandState (UnitStandStateType: SIT=1, DEAD=7, KNEEL=8)
-- GUIDs from 2026_13_32_05 (@CGUID 11001024+): +7 Win'sa, +8/+9 Horde Grunts
-- ---------------------------------------------------------------------------
DELETE FROM `creature_addon` WHERE `guid` IN (11001031, 11001032, 11001033);
INSERT INTO `creature_addon` (`guid`, `PathId`, `mount`, `MountCreatureID`, `StandState`, `AnimTier`, `VisFlags`, `SheathState`, `PvPFlags`, `emote`, `aiAnimKit`, `movementAnimKit`, `meleeAnimKit`, `visibilityDistanceType`, `auras`) VALUES
(11001031, 0, 0, 0, 8, 0, 0, 1, 0, 0, 0, 0, 0, 0, NULL), -- 245026 Win'sa @ -1089.58,-3545.22 — StandState KNEEL (sniff J)
(11001032, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, NULL), -- 245028 Horde Grunt @ -1088.34,-3542.44 — StandState SIT (sniff J)
(11001033, 0, 0, 0, 7, 0, 0, 1, 0, 0, 0, 0, 0, 0, NULL); -- 245028 Horde Grunt @ -1083.78,-3553.58 — StandState DEAD (sniff J)

-- ---------------------------------------------------------------------------
-- Pad scenes (PlaybackFlags from SMSG_PLAY_SCENE; ScriptPackageID from same)
-- Required for SpellAura 430 PLAY_SCENE on 1237116 / 1248494 (DB2 EffectMiscValue).
-- ---------------------------------------------------------------------------
DELETE FROM `scene_template` WHERE `SceneId` IN (3692, 3749);
INSERT INTO `scene_template` (`SceneId`, `Flags`, `ScriptPackageID`, `Encrypted`, `ScriptName`) VALUES
(3692, 16, 4617, 0, ''), -- ambient pad (sniff J/D/login); spell 1237116
(3749, 17, 4681, 0, ''); -- Jaina stasis presentation (sniff J/login); spell 1248494
