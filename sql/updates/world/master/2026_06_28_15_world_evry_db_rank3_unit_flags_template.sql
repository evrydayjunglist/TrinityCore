-- TrinityCore-evry DB Rank 3 phase 3A: creature_template unit_flags strip
-- Client build 12.0.7.68275
-- upstream-correct, non-hacky — persist ObjectMgr ALLOWED-mask strip
-- UNIT_FLAG_ALLOWED = 0x02002340 (UnitDefines.h L208)
-- UNIT_FLAG2_ALLOWED = 0x04034823 (UnitDefines.h L260)
-- UNIT_FLAG3_ALLOWED = 0x014DE0B6 (UnitDefines.h L310)
-- Pre-apply affected rows: 0
UPDATE `creature_template`
SET `unit_flags`  = `unit_flags`  & 0x02002340,
    `unit_flags2` = `unit_flags2` & 0x04034823,
    `unit_flags3` = `unit_flags3` & 0x014DE0B6
WHERE (`unit_flags`  & ~0x02002340)  != 0
   OR (`unit_flags2` & ~0x04034823) != 0
   OR (`unit_flags3` & ~0x014DE0B6) != 0;
