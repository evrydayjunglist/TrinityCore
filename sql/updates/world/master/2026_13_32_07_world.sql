-- Arathi RPE Phase 3b pose polish (sniff J `12-11-14` + Dracthyr sibling):
-- 1) Thrall / Jaina pad pose auras (retail CreateObject AURA_UPDATE + SpellVisual).
-- 2) Outdoor 245027 Assailants: stop idle-float "alive" look — Permanent Feign Death
--    like Dracthyr Talon Kethahn (181712 / aura 29266 in 2026_06_25_26), and clear the
--    FLOATING|SESSILE + AnimTier 3 presentation that made them hover as living floaters.
--    Stasis cloud remains scene 3749; dummies 249245 stay floating.
--
-- Evidence:
--   Thrall 244642 aura 1237057 (SpellVisual 483618) — sniff J
--   Jaina 244643 aura 1237118 (SpellVisual 483668) — sniff J
--   245027 CreateObject: StandState 0 (not DEAD); no Assailant aura in sniff J — death
--   presentation uses fork-proven Feign Death sibling (Dracthyr), owner playtest directed.

-- ---------------------------------------------------------------------------
-- Thrall / Jaina pose auras (creature_template_addon)
-- ---------------------------------------------------------------------------
DELETE FROM `creature_template_addon` WHERE `entry` IN (244642, 244643);
INSERT INTO `creature_template_addon` (`entry`, `PathId`, `mount`, `MountCreatureID`, `StandState`, `AnimTier`, `VisFlags`, `SheathState`, `PvPFlags`, `emote`, `aiAnimKit`, `movementAnimKit`, `meleeAnimKit`, `visibilityDistanceType`, `auras`) VALUES
(244642, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '1237057'), -- Thrall pad pose (sniff J)
(244643, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '1237118'); -- Jaina cast-hold pose (sniff J)

-- ---------------------------------------------------------------------------
-- Outdoor Assailants: Feign Death + ground presentation (keep 249245 floaters)
-- ---------------------------------------------------------------------------
UPDATE `creature_template_difficulty`
SET `StaticFlags1`=0,
    `VerifiedBuild`=68453
WHERE `Entry`=245027 AND `DifficultyID`=0;

DELETE FROM `creature_template_addon` WHERE `entry`=245027;
INSERT INTO `creature_template_addon` (`entry`, `PathId`, `mount`, `MountCreatureID`, `StandState`, `AnimTier`, `VisFlags`, `SheathState`, `PvPFlags`, `emote`, `aiAnimKit`, `movementAnimKit`, `meleeAnimKit`, `visibilityDistanceType`, `auras`) VALUES
(245027, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '29266'); -- Permanent Feign Death (Dracthyr Kethahn sibling)

-- Ensure training dummies keep float presentation if a prior DELETE touched them with 245027
DELETE FROM `creature_template_addon` WHERE `entry`=249245;
INSERT INTO `creature_template_addon` (`entry`, `PathId`, `mount`, `MountCreatureID`, `StandState`, `AnimTier`, `VisFlags`, `SheathState`, `PvPFlags`, `emote`, `aiAnimKit`, `movementAnimKit`, `meleeAnimKit`, `visibilityDistanceType`, `auras`) VALUES
(249245, 0, 0, 0, 0, 3, 4, 1, 0, 0, 0, 0, 0, 0, NULL); -- Training Dummy float (unchanged from 05)
