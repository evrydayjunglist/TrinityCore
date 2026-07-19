-- Arathi RPE Phase 3 follow-up: gate 90883 on 90882 (owner: both offered before gnoll turn-in).
-- quest_template_addon had no PrevQuestID rows for 90882–90887.

DELETE FROM `quest_template_addon` WHERE `ID`=90883;
INSERT INTO `quest_template_addon` (`ID`, `MaxLevel`, `AllowableClasses`, `SourceSpellID`, `PrevQuestID`, `NextQuestID`, `ExclusiveGroup`, `BreadcrumbForQuestId`, `RewardMailTemplateID`, `RewardMailDelay`, `RequiredSkillID`, `RequiredSkillPoints`, `RequiredMinRepFaction`, `RequiredMaxRepFaction`, `RequiredMinRepValue`, `RequiredMaxRepValue`, `ProvidedItemCount`, `SpecialFlags`, `ScriptName`) VALUES
(90883, 0, 0, 0, 90882, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '');
