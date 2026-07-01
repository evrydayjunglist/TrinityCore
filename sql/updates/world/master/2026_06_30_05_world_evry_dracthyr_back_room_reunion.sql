-- Dracthyr Forbidden Reach (B5): back-room "reunion" scene near Scalecommander Azurathel's
-- stasis spot. Retail sniff (`dump_12.0.7.68275_2026-06-30_18-15-43`) shows 3 more Dracthyr
-- clustered right next to Azurathel besides the player's personal guide: 186946 "Kodethi"
-- (skipped here — near-identical position to our existing personal-guide landing spot,
-- DRACTHYR_GUIDE_WAKE_WALK_TARGET in zone_the_forbidden_reach.cpp, so adding a second, distinct
-- Kodethi would just look like a visual duplicate), 181596 "Dervishian", and 186864 "Tethalash".
-- All three are distinct GUIDs/entries from the personal/per-quest copies (187223/181494 guide,
-- 181680 Lower War Creche Tethalash) — always present from early in the capture, well before the
-- player's personal Tethalash is ever freed; no wire evidence of the personal Tethalash walking
-- the ~120yd from the Lower War Creche up to this room was found (see handoff §B5 for the
-- OutOfRange/create-object timing analysis). 186864 already speaks MonsterSay 216876 ("I
-- remember... obeying an order.") ~2.3s after the disintegrate hit that frees personal Tethalash;
-- 181056 (Azurathel, already-existing quest-ender entry) speaks 213820 ("Ungh... my head...
-- Alright, we need to get our bearings.") ~1.2s after her own hit. Both broadcast texts already
-- exist in base client BroadcastText.db2 — only the creature_text linking rows are fork content.
--
-- See docs/midnight-assessment/dracthyr/dracthyr-forbidden-reach-handoff.md §B5.

SET @CGUID := 9003904;

DELETE FROM `creature` WHERE `guid` BETWEEN @CGUID+0 AND @CGUID+1;
INSERT INTO `creature` (`guid`, `id`, `map`, `zoneId`, `areaId`, `spawnDifficulties`, `PhaseId`, `modelid`, `equipment_id`, `position_x`, `position_y`, `position_z`, `orientation`, `spawntimesecs`, `wander_distance`, `currentwaypoint`, `curHealthPct`, `MovementType`, `ScriptName`, `StringId`, `VerifiedBuild`) VALUES
(@CGUID+0, 181596, 2570, 13769, 13806, '0', 0, 0, 0, 5815.007, -2915.8074, 207.30219, 2.7634056, 120, 0, 0, 100, 0, '', NULL, 68275), -- Dervishian (already-there back-room dracthyr)
(@CGUID+1, 186864, 2570, 13769, 13806, '0', 0, 0, 0, 5803.7197, -2921.448, 207.1868, 2.7634056, 120, 0, 0, 100, 0, '', NULL, 68275); -- Tethalash (reunited, back room)

-- 181596 defaults to faction 0 (Monster) in base client data; 186864 already ships with the
-- correct friendly faction (35, matching Kethahn/Tethalash/Azurathel) so needs no template edit.
UPDATE `creature_template` SET `faction`=35 WHERE `entry`=181596;

-- Wake-up MonsterSay lines (GroupID 0), timed from zone_the_forbidden_reach.cpp's
-- DracthyrTethalashWakeEvent / DracthyrAzurathelWakeSpeakEvent.
DELETE FROM `creature_text` WHERE `CreatureID` IN (186864, 181056) AND `GroupID`=0;
INSERT INTO `creature_text` (`CreatureID`, `GroupID`, `ID`, `Text`, `Type`, `Language`, `Probability`, `Emote`, `Duration`, `Sound`, `SoundPlayType`, `BroadcastTextId`, `TextRange`, `comment`) VALUES
(186864, 0, 0, 'I remember... obeying an order.', 12, 0, 100, 0, 0, 0, 0, 216876, 0, 'Tethalash (reunited) - wake-up MonsterSay'),
(181056, 0, 0, 'Ungh... my head... Alright, we need to get our bearings.', 12, 0, 100, 0, 0, 0, 0, 213820, 0, 'Scalecommander Azurathel - wake-up MonsterSay');
