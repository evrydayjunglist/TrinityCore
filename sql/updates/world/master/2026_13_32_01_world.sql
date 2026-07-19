-- Arathi RPE (map 2927) Phase 3 questline vertical slice:
-- keep gnolls (90882 credit) → turn-in / LaunchQuest 90883 → farm pad + openers 90885/86/87.
-- Evidence: retail sniffs D/H build 12.0.7.68453 (Desktop hello/ dump_*_01-07-08 / *_01-18-00).
-- KillCredit1 already maps 244669/244670/244671 → ObjectID 244672 (quest_objectives Amount 10).

SET @CGUID := 11001002;

-- ---------------------------------------------------------------------------
-- Hostile keep gnolls (faction 16 matches existing 244671 template)
-- ---------------------------------------------------------------------------
UPDATE `creature_template` SET `faction`=16 WHERE `entry` IN (244669, 244670, 244672) AND `faction`=0;

-- Keep cluster CreateObject positions from sniffs D/H (map 2927 / zone 16432)
DELETE FROM `creature` WHERE `guid` BETWEEN @CGUID+0 AND @CGUID+21;
INSERT INTO `creature` (`guid`, `id`, `map`, `zoneId`, `areaId`, `spawnDifficulties`, `PhaseId`, `PhaseGroup`, `modelid`, `equipment_id`, `position_x`, `position_y`, `position_z`, `orientation`, `spawntimesecs`, `wander_distance`, `currentwaypoint`, `MovementType`, `npcflag`, `unit_flags`, `unit_flags2`, `unit_flags3`, `VerifiedBuild`) VALUES
-- 244672 Gnoll Bruiser (objective ObjectID; sample ≈ -996.47,-3527.87,56.99)
(@CGUID+0,  244672, 2927, 16432, 16432, '0', 0, 0, 0, 0, -981.45026, -3513.761,  56.992092, 3.3214676, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 68453),
(@CGUID+1,  244672, 2927, 16432, 16432, '0', 0, 0, 0, 0, -988.5839,  -3518.1963, 56.992092, 4.0285850, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 68453),
(@CGUID+2,  244672, 2927, 16432, 16432, '0', 0, 0, 0, 0, -994.482,   -3525.4333, 56.992092, 0,         120, 0, 0, 0, NULL, NULL, NULL, NULL, 68453),
(@CGUID+3,  244672, 2927, 16432, 16432, '0', 0, 0, 0, 0, -999.4901,  -3530.6072, 56.98025,  0,         120, 0, 0, 0, NULL, NULL, NULL, NULL, 68453),
(@CGUID+4,  244672, 2927, 16432, 16432, '0', 0, 0, 0, 0, -1005.5139, -3536.0608, 56.57396,  0,         120, 0, 0, 0, NULL, NULL, NULL, NULL, 68453),
(@CGUID+5,  244672, 2927, 16432, 16432, '0', 0, 0, 0, 0, -996.47,    -3527.87,   56.99,     0,         120, 0, 0, 0, NULL, NULL, NULL, NULL, 68453),
(@CGUID+6,  244672, 2927, 16432, 16432, '0', 0, 0, 0, 0, -975.55646, -3512.6892, 56.992092, 0,         120, 0, 0, 0, NULL, NULL, NULL, NULL, 68453),
(@CGUID+7,  244672, 2927, 16432, 16432, '0', 0, 0, 0, 0, -972.067,   -3511.9495, 56.992092, 0,         120, 0, 0, 0, NULL, NULL, NULL, NULL, 68453),
(@CGUID+8,  244672, 2927, 16432, 16432, '0', 0, 0, 0, 0, -962.2465,  -3509.7275, 56.992096, 0,         120, 0, 0, 0, NULL, NULL, NULL, NULL, 68453),
-- 244669 / 244670 / 244671 (KillCredit1 → 244672; D/H CreateObject)
(@CGUID+9,  244669, 2927, 16432, 16432, '0', 0, 0, 0, 0, -1016.804,  -3519.98,   61.393707, 3.3338888, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 68453),
(@CGUID+10, 244669, 2927, 16432, 16432, '0', 0, 0, 0, 0, -1020.7656, -3517.7917, 61.757744, 0,         120, 0, 0, 0, NULL, NULL, NULL, NULL, 68453),
(@CGUID+11, 244669, 2927, 16432, 16432, '0', 0, 0, 0, 0, -1015.9045, -3519.804,  61.477505, 3.5282719, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 68453),
(@CGUID+12, 244670, 2927, 16432, 16432, '0', 0, 0, 0, 0, -1013.5469, -3574.7432, 56.647884, 5.3338089, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 68453),
(@CGUID+13, 244670, 2927, 16432, 16432, '0', 0, 0, 0, 0, -1014.908,  -3516.7205, 61.730278, 3.7660933, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 68453),
(@CGUID+14, 244671, 2927, 16432, 16432, '0', 0, 0, 0, 0, -1033.3021, -3551.8108, 56.267727, 3.1737113, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 68453),
(@CGUID+15, 244671, 2927, 16432, 16432, '0', 0, 0, 0, 0, -1010.8646, -3563.9548, 56.647884, 1.6648406, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 68453),
(@CGUID+16, 244671, 2927, 16432, 16432, '0', 0, 0, 0, 0, -1025.5286, -3489.8262, 62.304573, 0.7926827, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 68453),
(@CGUID+17, 244671, 2927, 16432, 16432, '0', 0, 0, 0, 0, -985.0482,  -3541.8672, 56.992737, 0.9016410, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 68453),
-- Farm pad (sniff H) — Bruvk / Thrall / Jaina
(@CGUID+18, 244729, 2927, 16432, 16432, '0', 0, 0, 0, 0, -1522.33,   -3089.36,   26.34,     2.175,     120, 0, 0, 0, NULL, NULL, NULL, NULL, 68453), -- Farmer Bruvk
(@CGUID+19, 244656, 2927, 16432, 16432, '0', 0, 0, 0, 0, -1522.62,   -3085.87,   26.17,     1.533,     120, 0, 0, 0, NULL, NULL, NULL, NULL, 68453), -- Thrall (farm)
(@CGUID+20, 244655, 2927, 16432, 16432, '0', 0, 0, 0, 0, -1525.88,   -3089.80,   26.12,     3.182,     120, 0, 0, 0, NULL, NULL, NULL, NULL, 68453), -- Lady Jaina Proudmoore (farm)
-- Extra 244672 for kill-count headroom (respawn / spread)
(@CGUID+21, 244672, 2927, 16432, 16432, '0', 0, 0, 0, 0, -983.041,   -3514.0503, 56.992092, 0,         120, 0, 0, 0, NULL, NULL, NULL, NULL, 68453);

-- Questgiver flags on farm pad (Bruvk/Thrall/Jaina)
UPDATE `creature_template` SET `npcflag`=`npcflag`|2 WHERE `entry` IN (244729, 244656, 244655);
-- Bruvk was faction 0 (unusable); friendly ambient like farm Jaina
UPDATE `creature_template` SET `faction`=35 WHERE `entry`=244729 AND `faction`=0;

-- ---------------------------------------------------------------------------
-- Phase 2 debt: Horde saw dual ! on Jaina+Thrall — drop Jaina as 90882 starter/ender
-- (Alliance-only per-NPC starter not available via conditions; Alliance re-wire later)
-- ---------------------------------------------------------------------------
DELETE FROM `creature_queststarter` WHERE `quest`=90882 AND `id`=244643;
DELETE FROM `creature_questender` WHERE `quest`=90882 AND `id`=244643;

-- ---------------------------------------------------------------------------
-- 90882 → 90883 → farm openers (starters/enders from sniff H)
-- RewardNextQuest: QUERY showed 0; set for TC GetNextQuest / LaunchQuest details path
-- matching sniff LaunchQuest: True → next quest on same giver.
-- ---------------------------------------------------------------------------
UPDATE `quest_template` SET `RewardNextQuest`=90883 WHERE `ID`=90882 AND `RewardNextQuest`=0;
UPDATE `quest_template` SET `RewardNextQuest`=90885 WHERE `ID`=90883 AND `RewardNextQuest`=0;

DELETE FROM `creature_queststarter` WHERE `quest`=90883 AND `id`=244642;
INSERT INTO `creature_queststarter` (`id`, `quest`, `VerifiedBuild`) VALUES
(244642, 90883, 68453); -- To Go'shek Farm — Thrall (Hammerfall)

DELETE FROM `creature_questender` WHERE `quest`=90883 AND `id`=244729;
INSERT INTO `creature_questender` (`id`, `quest`, `VerifiedBuild`) VALUES
(244729, 90883, 68453); -- turn-in Farmer Bruvk

DELETE FROM `creature_queststarter` WHERE `quest` IN (90885, 90886, 90887) AND `id` IN (244729, 244656, 244655);
INSERT INTO `creature_queststarter` (`id`, `quest`, `VerifiedBuild`) VALUES
(244729, 90885, 68453), -- My Beautiful Pumpkins — Bruvk
(244656, 90886, 68453), -- Best Laid Plans… — farm Thrall
(244655, 90887, 68453); -- Farmer's Nemesis — farm Jaina

-- Enders for farm openers: keep pre-existing 64978 rows (244656→90885/86, 244655→90887);
-- also allow Bruvk to end 90885 (same giver as accept; sniff complete path not captured)
DELETE FROM `creature_questender` WHERE `quest`=90885 AND `id`=244729;
INSERT INTO `creature_questender` (`id`, `quest`, `VerifiedBuild`) VALUES
(244729, 90885, 68453);

-- ---------------------------------------------------------------------------
-- Phase shifts (sniff H) — CONDITION_SOURCE_TYPE_PHASE + CONDITION_QUESTSTATE
-- Login: 26596, 26618, 27217
-- 90882 rewarded → drop 26618; 90883 taken → drop 26596; farm → 26588/26599
-- ---------------------------------------------------------------------------
DELETE FROM `phase_name` WHERE `ID` IN (26588, 26599);
INSERT INTO `phase_name` (`ID`, `Name`) VALUES
(26588, 'Arathi RPE - Go''shek Farm arrive (sniff H)'),
(26599, 'Arathi RPE - Go''shek Farm persistent (sniff H)');

DELETE FROM `phase_area` WHERE `PhaseId` IN (26588, 26599) AND `AreaId`=16432;
INSERT INTO `phase_area` (`AreaId`, `PhaseId`, `Comment`) VALUES
(16432, 26588, 'Arathi RPE - farm arrive phase (sniff H)'),
(16432, 26599, 'Arathi RPE - farm persistent phase (sniff H)');

-- 26618: only before 90882 is rewarded (removed on QUEST_UPDATE_COMPLETE 90882)
DELETE FROM `conditions` WHERE `SourceTypeOrReferenceId`=26 AND `SourceGroup` IN (26618, 26596, 26588, 26599) AND `SourceEntry`=0;
INSERT INTO `conditions` (`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `SourceId`, `ElseGroup`, `ConditionTypeOrReference`, `ConditionTarget`, `ConditionValue1`, `ConditionValue2`, `ConditionValue3`, `NegativeCondition`, `Comment`) VALUES
(26, 26618, 0, 0, 0, 47, 0, 90882, 64, 0, 1, 'Arathi RPE: phase 26618 while 90882 not rewarded (sniff H)'),
-- 26596: only before 90883 is taken (removed on accept 90883)
(26, 26596, 0, 0, 0, 47, 0, 90883, 74, 0, 1, 'Arathi RPE: phase 26596 while 90883 not incomplete|complete|rewarded (sniff H)'),
-- 26588: after 90883 started/done, until 90885 accepted
(26, 26588, 0, 0, 0, 47, 0, 90883, 74, 0, 0, 'Arathi RPE: phase 26588 if 90883 incomplete|complete|rewarded (sniff H)'),
(26, 26588, 0, 0, 0, 47, 0, 90885, 74, 0, 1, 'Arathi RPE: phase 26588 while 90885 not incomplete|complete|rewarded (sniff H)'),
-- 26599: once 90883 segment reached (stays after 90885 accept)
(26, 26599, 0, 0, 0, 47, 0, 90883, 74, 0, 0, 'Arathi RPE: phase 26599 if 90883 incomplete|complete|rewarded (sniff H)');

-- 27217 remains unconditional on area 16432 (Phase 2 seed) — persistent through farm.
