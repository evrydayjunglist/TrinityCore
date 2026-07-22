-- Archaeology Phase 2C (Forward option 2): provisional branch-keystone find loot.
-- Identity from ResearchBranch.ItemID; quantity one; Chance 7 is provisional
-- reference-backed debt (historical BFADB/BfaCore shared shape; compatible with retained
-- 12.0.7.68453 batch ~10/90). Not a retail-configured percentage.
-- Fossil (branch 3) has ItemID 0 — no keystone row.
--
-- SQL fuse for a future authoritative rate (one line):
--   UPDATE `gameobject_loot_template`
--   SET `Chance` = <retail>
--   WHERE `Item` IN (52843, 63127, 64396, 64397, 63128)
--     AND `Comment` LIKE '%provisional keystone%';
--
-- Dual-check: free NN `10` on master + evry for 2026_13_32 world (2026-07-22).

DELETE FROM `gameobject_loot_template`
WHERE `Entry` IN (28434, 35546, 35827, 36055, 36056)
  AND `ItemType` = 0
  AND `Item` IN (52843, 63127, 64396, 64397, 63128);

INSERT INTO `gameobject_loot_template`
(`Entry`,`ItemType`,`Item`,`Chance`,`QuestRequired`,`LootMode`,`GroupId`,`MinCount`,`MaxCount`,`Comment`) VALUES
(28434, 0, 52843, 7, 0, 1, 0, 1, 1, 'Dwarf Rune Stone (provisional keystone chance)'),
(35827, 0, 63127, 7, 0, 1, 0, 1, 1, 'Highborne Scroll (provisional keystone chance)'),
(36055, 0, 64396, 7, 0, 1, 0, 1, 1, 'Nerubian Obelisk (provisional keystone chance)'),
(36056, 0, 64397, 7, 0, 1, 0, 1, 1, 'Tol''vir Hieroglyphic (provisional keystone chance)'),
(35546, 0, 63128, 7, 0, 1, 0, 1, 1, 'Troll Tablet (provisional keystone chance)');
