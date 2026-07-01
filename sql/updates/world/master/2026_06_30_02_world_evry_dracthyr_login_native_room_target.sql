-- Dracthyr Forbidden Reach: drive the first-login room teleport (369728, EFFECT_0) through
-- TrinityCore's native TARGET_DEST_NEARBY_DB targeting instead of a hand-rolled C++ urand()
-- pick. DB2 evidence (12.0.7.67808 SpellEffect export): 369728 EFFECT_0 implicit target is
-- TARGET_DEST_NEARBY_DB (106) with EffectRadiusIndex -> SpellRadius.db2 ID 30 = 500 yards,
-- comfortably covering all four War Creche rooms from the hub spawn. This is retail's actual
-- "room rotation" mechanism (Spell.cpp TARGET_DEST_NEARBY_DB handler: collect
-- `spell_target_position` rows in range, pick one at random).
--
-- Coordinates match `LoginRoomData` in zone_the_forbidden_reach.cpp (PlayerPosition), already
-- independently sniff/playtest-validated as the correct four room spawn points.
--
-- See docs/midnight-assessment/dracthyr/dracthyr-intro-opening-retail-parity-handoff.md §5c.

DELETE FROM `spell_target_position` WHERE `ID` = 369728 AND `EffectIndex` = 0;
INSERT INTO `spell_target_position` (`ID`, `EffectIndex`, `OrderIndex`, `MapID`, `PositionX`, `PositionY`, `PositionZ`, `Orientation`, `VerifiedBuild`) VALUES
(369728, 0, 0, 2570, 5725.32,  -3024.26, 251.047, 0.01745329238474369,  68275),
(369728, 0, 1, 2570, 5743.03,  -3067.28, 251.047, 0.798488140106201171, 68275),
(369728, 0, 2, 2570, 5787.1597,-3083.3906,251.04698,1.570796370506286621,68275),
(369728, 0, 3, 2570, 5829.32,  -3064.49, 251.047, 2.364955902099609375, 68275);
