-- TrinityCore-evry DB Rank 3 phase 3B: creature spawn unit_flags overrides strip
-- Client build 12.0.7.68275
-- upstream-correct, non-hacky — persist ObjectMgr ALLOWED-mask strip
-- UNIT_FLAG_ALLOWED = 0x02002340 (UnitDefines.h L208)
-- UNIT_FLAG2_ALLOWED = 0x04034823 (UnitDefines.h L260)
-- UNIT_FLAG3_ALLOWED = 0x014DE0B6 (UnitDefines.h L310)
-- Pre-apply affected rows: 0
UPDATE `creature`
SET `unit_flags`  = CASE WHEN `unit_flags`  IS NOT NULL THEN `unit_flags`  & 0x02002340  ELSE NULL END,
    `unit_flags2` = CASE WHEN `unit_flags2` IS NOT NULL THEN `unit_flags2` & 0x04034823 ELSE NULL END,
    `unit_flags3` = CASE WHEN `unit_flags3` IS NOT NULL THEN `unit_flags3` & 0x014DE0B6 ELSE NULL END
WHERE (`unit_flags`  IS NOT NULL AND (`unit_flags`  & ~0x02002340)  != 0)
   OR (`unit_flags2` IS NOT NULL AND (`unit_flags2` & ~0x04034823) != 0)
   OR (`unit_flags3` IS NOT NULL AND (`unit_flags3` & ~0x014DE0B6) != 0);
