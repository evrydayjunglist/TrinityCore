-- TrinityCore-evry DB Rank 3 phase 3C: creature_template flags_extra strip
-- Client build 12.0.7.68275
-- upstream-correct, non-hacky — persist ObjectMgr ALLOWED-mask strip
-- CREATURE_FLAG_EXTRA_DB_ALLOWED = 0x603FFFFF (CreatureData.h L382)
-- Pre-apply affected rows: 0
UPDATE `creature_template`
SET `flags_extra` = `flags_extra` & 0x603FFFFF
WHERE (`flags_extra` & ~0x603FFFFF) != 0;
