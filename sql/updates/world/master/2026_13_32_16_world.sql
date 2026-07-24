-- Chromie Time: renumber CONDITION_CHROMIE_TIME 60 -> 61
-- Upstream claimed CONDITION_GROUP_STATUS = 60; Chromie moves to 61.
-- Flow 2R dual-check PASS: NN=16 free on master+evry (tip had world 00-15).
-- Leaves 2026_13_32_11_world.sql unchanged (already shipped); remaps applied rows.

UPDATE `conditions`
SET `ConditionTypeOrReference` = 61
WHERE `SourceTypeOrReferenceId` = 15
  AND `SourceGroup` = 25426
  AND `SourceEntry` IN (0, 1, 2)
  AND `ConditionTypeOrReference` = 60;
