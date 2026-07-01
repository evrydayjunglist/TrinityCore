-- Dracthyr Forbidden Reach (B3): disable Disintegrate spellclick on Tethalash after awaken
-- (objective 423680 credited). Mirrors 2026_06_25_27 (Kodethi/Dervishian polish).
-- See docs/midnight-assessment/dracthyr/dracthyr-phase-1b-handoff.md §11 B3.

DELETE FROM `conditions` WHERE `SourceTypeOrReferenceId`=18 AND `SourceGroup`=181680 AND `SourceEntry`=362355 AND `SourceId`=0 AND `ConditionTypeOrReference`=48;
INSERT INTO `conditions` (`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `SourceId`, `ElseGroup`, `ConditionTypeOrReference`, `ConditionTarget`, `ConditionValue1`, `ConditionValue2`, `ConditionValue3`, `NegativeCondition`, `ErrorType`, `ErrorTextId`, `ScriptName`, `Comment`) VALUES
(18, 181680, 362355, 0, 0, 48, 0, 423680, 0, 0, 0, 0, 0, '', 'Spellclick Tethalash: Disintegrate only while quest 64864 objective 423680 incomplete');
