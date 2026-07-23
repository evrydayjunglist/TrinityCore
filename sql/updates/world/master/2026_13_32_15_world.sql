-- Archaeology Phase 3A: enable Mantid/Pandaren/Mogu find policy + fragment loot.
-- Find GOs re-verified in local world gameobject_template (chest, lock 1859):
--   211163 Pandaren Archaeology Find (data1=41486)
--   211174 Mogu Archaeology Find (data1=41505)
--   218950 Mantid Archaeology Find (data1=46918)
-- Wowhead mop-classic object pages CDN-degraded this pass; world names/types match prior
-- Phase 3 freeze leads (BfaCore + Wowhead for 211163/211174; Mantid world-DB re-verify).
-- Fragment currencies from ResearchBranch.db2 CurrencyID (754/676/677).
-- Loot template IDs from gameobject_template.data1.
-- Provisional 7% keystone fuse not extended in this slice (owner gate).
-- Dual-check: free NN 15 on master + evry for 2026_13_32 world (2026-07-22);
-- tip already owns 10-14; master/evry max was 09.

DELETE FROM `archaeology_research_branch` WHERE `researchBranchId` IN (29,229,231);
INSERT INTO `archaeology_research_branch` (`researchBranchId`,`findGameObjectId`) VALUES
(29,218950),  -- Mantid
(229,211163), -- Pandaren
(231,211174); -- Mogu

UPDATE `gameobject_template`
SET `ScriptName` = 'go_archaeology_find',
    `data1` = CASE `entry`
      WHEN 211163 THEN 41486 -- Pandaren
      WHEN 211174 THEN 41505 -- Mogu
      WHEN 218950 THEN 46918 -- Mantid
    END
WHERE `entry` IN (211163,211174,218950);

DELETE FROM `gameobject_loot_template`
WHERE `Entry` IN (41486,41505,46918)
  AND `ItemType` = 2
  AND `Item` IN (676,677,754);
INSERT INTO `gameobject_loot_template`
(`Entry`,`ItemType`,`Item`,`Chance`,`QuestRequired`,`LootMode`,`GroupId`,`MinCount`,`MaxCount`,`Comment`) VALUES
(41486,2,676,100,0,1,0,5,9,'Pandaren Archaeology Fragments'),
(41505,2,677,100,0,1,0,5,9,'Mogu Archaeology Fragments'),
(46918,2,754,100,0,1,0,5,9,'Mantid Archaeology Fragments');
