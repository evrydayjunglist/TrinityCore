-- Arathi RPE (map 2927) Phase 2 world scaffolding: Hammerfall pad starters + 90882 offer.
-- Evidence: retail sniffs D/H build 12.0.7.68453 (Thrall 244642 / Jaina 244643 @ pad; quest 90882).
-- Zone 16432 from sniff regionwide / login; PhaseIds 26596/26618/27217 from H PhaseShift on RPE enter.
-- Creature PhaseId 0: Unphased visibility for first interactable (player also gets cosmetic phases via phase_area when area resolves).

SET @CGUID := 11001000;

-- Creatures (Hammerfall pad — CreateObject positions from sniff H)
DELETE FROM `creature` WHERE `guid` BETWEEN @CGUID+0 AND @CGUID+1;
INSERT INTO `creature` (`guid`, `id`, `map`, `zoneId`, `areaId`, `spawnDifficulties`, `PhaseId`, `PhaseGroup`, `modelid`, `equipment_id`, `position_x`, `position_y`, `position_z`, `orientation`, `spawntimesecs`, `wander_distance`, `currentwaypoint`, `MovementType`, `npcflag`, `unit_flags`, `unit_flags2`, `unit_flags3`, `VerifiedBuild`) VALUES
(@CGUID+0, 244642, 2927, 16432, 16432, '0', 0, 0, 0, 0, -1086.4791, -3554.7744, 50.192024, 0.09973325, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 68453), -- Thrall (Map: 2927 Arathi Highlands RPE)
(@CGUID+1, 244643, 2927, 16432, 16432, '0', 0, 0, 0, 0, -1084.2153, -3559.9722, 50.44527, 5.0222363, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 68453); -- Lady Jaina Proudmoore (Map: 2927 Arathi Highlands RPE)

-- Questgiver flag (sniff H: NpcFlags 2 on Thrall)
UPDATE `creature_template` SET `npcflag`=`npcflag`|2 WHERE `entry` IN (244642, 244643);

-- 90882 Gnoll Way — Horde giver Thrall; Jaina present for Alliance text variants (D/H)
DELETE FROM `creature_queststarter` WHERE `quest`=90882 AND `id` IN (244642, 244643);
INSERT INTO `creature_queststarter` (`id`, `quest`, `VerifiedBuild`) VALUES
(244642, 90882, 68453), -- Gnoll Way offered by Thrall
(244643, 90882, 68453); -- Gnoll Way offered by Lady Jaina Proudmoore

DELETE FROM `creature_questender` WHERE `quest`=90882 AND `id` IN (244642, 244643);
INSERT INTO `creature_questender` (`id`, `quest`, `VerifiedBuild`) VALUES
(244642, 90882, 68453),
(244643, 90882, 68453);

-- Seed RPE login cosmetic phases (sniff H PhaseShift on map 2927 enter)
DELETE FROM `phase_name` WHERE `ID` IN (26596, 26618, 27217);
INSERT INTO `phase_name` (`ID`, `Name`) VALUES
(26596, 'Arathi RPE - Hammerfall login (sniff H)'),
(26618, 'Arathi RPE - Hammerfall login (sniff H)'),
(27217, 'Arathi RPE - persistent through farm (sniff H)');

DELETE FROM `phase_area` WHERE `PhaseId` IN (26596, 26618, 27217) AND `AreaId`=16432;
INSERT INTO `phase_area` (`AreaId`, `PhaseId`, `Comment`) VALUES
(16432, 26596, 'Arathi RPE zone - login phases (sniff H)'),
(16432, 26618, 'Arathi RPE zone - login phases (sniff H)'),
(16432, 27217, 'Arathi RPE zone - login phases (sniff H)');
