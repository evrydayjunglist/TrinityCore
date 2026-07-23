-- Archaeology Phase 2F nine-branch: provisional keystone find loot for Draenei/Orc/Vrykul.
-- Identity from ResearchBranch.ItemID (64394/64392/64395); quantity one; Chance 7 matches the
-- Phase 2C provisional fuse (and historical Jade/BFADB shared shape). Not a retail-configured %.
--
-- SQL fuse for a future authoritative rate (extend the six-branch fuse):
--   UPDATE `gameobject_loot_template`
--   SET `Chance` = <retail>
--   WHERE `Item` IN (52843, 63127, 64396, 64397, 63128, 64392, 64394, 64395)
--     AND `Comment` LIKE '%provisional keystone%';
--
-- Dual-check: free NN `12` on master + evry for 2026_13_32 world (2026-07-22).

DELETE FROM `gameobject_loot_template`
WHERE `Entry` IN (36019, 36037, 36054)
  AND `ItemType` = 0
  AND `Item` IN (64392, 64394, 64395);

INSERT INTO `gameobject_loot_template`
(`Entry`,`ItemType`,`Item`,`Chance`,`QuestRequired`,`LootMode`,`GroupId`,`MinCount`,`MaxCount`,`Comment`) VALUES
(36019, 0, 64392, 7, 0, 1, 0, 1, 1, 'Orc Blood Text (provisional keystone chance)'),
(36037, 0, 64394, 7, 0, 1, 0, 1, 1, 'Draenei Tome (provisional keystone chance)'),
(36054, 0, 64395, 7, 0, 1, 0, 1, 1, 'Vrykul Rune Stick (provisional keystone chance)');
