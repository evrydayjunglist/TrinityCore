-- Archaeology Phase 2F nine-branch: enable Draenei/Orc/Vrykul find policy + fragment loot.
-- Find GOs verified in local world gameobject_template (chest, lock 1859) and Wowhead object pages
-- (207187 Orc, 207188 Draenei, 207189 Vrykul). Jade IDs were leads only.
-- Fragment currencies from ResearchBranch.db2 CurrencyID (398/397/399).
-- Loot template IDs from gameobject_template.data1 (36037/36019/36054).
-- Dual-check: free NN `11` on master + evry for 2026_13_32 world (2026-07-22).

DELETE FROM `archaeology_research_branch` WHERE `researchBranchId` IN (2,6,27);
INSERT INTO `archaeology_research_branch` (`researchBranchId`,`findGameObjectId`) VALUES
(2,207188), -- Draenei
(6,207187), -- Orc
(27,207189); -- Vrykul

UPDATE `gameobject_template`
SET `ScriptName` = 'go_archaeology_find',
    `data1` = CASE `entry`
      WHEN 207187 THEN 36019 -- Orc
      WHEN 207188 THEN 36037 -- Draenei
      WHEN 207189 THEN 36054 -- Vrykul
    END
WHERE `entry` IN (207187,207188,207189);

DELETE FROM `gameobject_loot_template`
WHERE `Entry` IN (36019,36037,36054)
  AND `ItemType` = 2
  AND `Item` IN (397,398,399);
INSERT INTO `gameobject_loot_template`
(`Entry`,`ItemType`,`Item`,`Chance`,`QuestRequired`,`LootMode`,`GroupId`,`MinCount`,`MaxCount`,`Comment`) VALUES
(36019,2,397,100,0,1,0,5,9,'Orc Archaeology Fragments'),
(36037,2,398,100,0,1,0,5,9,'Draenei Archaeology Fragments'),
(36054,2,399,100,0,1,0,5,9,'Vrykul Archaeology Fragments');
