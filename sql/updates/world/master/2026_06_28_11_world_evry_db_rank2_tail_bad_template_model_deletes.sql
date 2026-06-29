-- TrinityCore-evry DB Rank 2 tail Phase 2E: bad creature_template_model DELETE
-- Build 12.0.7.68275 — CreatureDisplayID not in CreatureDisplayInfo.db2
-- Evidence: creaturedisplayinfo_12_0_7_68275.csv + runtime layout 0x7275F5F6
-- Distinct CreatureDisplayIDs: 36 (48 rows)
-- Zero-model entries after DELETE: 43 (see temp/rank2-tail-zero-model-entries.csv)
-- Label: retail-like / non-hacky

DELETE FROM `creature_template_model` WHERE `CreatureDisplayID` IN (959, 1089, 1090, 1951, 12346, 15939, 18287, 19338, 20375, 22588, 23495, 23504, 25152, 25452, 26590, 27015, 27016, 27017, 27029, 29525, 30521, 32578, 33427, 34276, 35014, 35080, 35231, 36130, 36751, 37413, 38043, 38961, 52963, 58640, 60754, 85414);
