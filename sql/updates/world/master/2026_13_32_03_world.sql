-- Arathi RPE Phase 3 polish: keep gnolls used ContentTuningID 0 → SelectLevel collapsed to 1
-- (owner playtest: Pallyfriend 80 vs lvl-1 gnolls). Quests 90882–90887 use ContentTuningID 4306;
-- ContentTuning.db2 12.0.7.68453 ID 4306: MinLevelSquish 10, MaxLevelScalingOffset 2 (MaxLevel).

UPDATE `creature_template_difficulty`
SET `ContentTuningID`=4306,
    `VerifiedBuild`=68453
WHERE `Entry` IN (244669, 244670, 244671, 244672)
  AND `DifficultyID`=0
  AND `ContentTuningID`=0;
