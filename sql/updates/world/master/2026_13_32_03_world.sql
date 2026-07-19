-- TreasurePicker quest rewards (server-side picker contents)
-- Evidence: retail sniff H 12.0.7.68453 (dump_12.0.7.68453_2026-07-19_01-18-00_parsed.txt)
-- No TreasurePicker*.db2 in client/WoWDBDefs — contents are server-authoritative.

CREATE TABLE IF NOT EXISTS `treasure_picker` (
  `TreasurePickerID` int unsigned NOT NULL,
  `Flags` int NOT NULL DEFAULT '0',
  `IsChoice` tinyint unsigned NOT NULL DEFAULT '0',
  `Gold` bigint unsigned NOT NULL DEFAULT '0',
  `VerifiedBuild` int NOT NULL DEFAULT '0',
  PRIMARY KEY (`TreasurePickerID`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS `treasure_picker_items` (
  `TreasurePickerID` int unsigned NOT NULL,
  `Idx` int unsigned NOT NULL,
  `ItemID` int unsigned NOT NULL DEFAULT '0',
  `ItemQuantity` int unsigned NOT NULL DEFAULT '1',
  `BonusListID` int NOT NULL DEFAULT '0',
  `Context` tinyint unsigned NOT NULL DEFAULT '0',
  `VerifiedBuild` int NOT NULL DEFAULT '0',
  PRIMARY KEY (`TreasurePickerID`,`Idx`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

DELETE FROM `treasure_picker` WHERE `TreasurePickerID` IN (4025, 4026, 4027, 4028, 4029);
INSERT INTO `treasure_picker` (`TreasurePickerID`, `Flags`, `IsChoice`, `Gold`, `VerifiedBuild`) VALUES
(4025, 128, 0, 0, 68453), -- 90882 Gnoll Way
(4026, 0, 0, 0, 68453),   -- 90883 To Go'shek Farm
(4027, 128, 0, 0, 68453), -- 90885 My Beautiful Pumpkins
(4028, 128, 0, 0, 68453), -- 90887 Farmer's Nemesis
(4029, 128, 0, 0, 68453); -- 90886 Best Laid Plans…

DELETE FROM `treasure_picker_items` WHERE `TreasurePickerID` IN (4025, 4026, 4027, 4028, 4029);
INSERT INTO `treasure_picker_items` (`TreasurePickerID`, `Idx`, `ItemID`, `ItemQuantity`, `BonusListID`, `Context`, `VerifiedBuild`) VALUES
-- 4025 / 90882 — grant ItemReward 153973 + BonusListID 4790 (Context 17); WPP expands CT 4306 / TW 80 from bonus 4790
(4025, 0, 153973, 1, 4790, 17, 68453),
(4025, 1, 153983, 1, 4790, 17, 68453),
(4025, 2, 153983, 1, 4790, 17, 68453),
(4025, 3, 154005, 1, 4790, 17, 68453),
-- 4026 / 90883 — grant ItemReward 249773
(4026, 0, 249773, 1, 0, 0, 68453),
(4026, 1, 249772, 1, 0, 0, 68453),
(4026, 2, 249771, 1, 0, 0, 68453),
(4026, 3, 188213, 1, 0, 0, 68453),
-- 4027 / 90885
(4027, 0, 153996, 1, 4790, 17, 68453),
(4027, 1, 153995, 1, 4790, 17, 68453),
-- 4028 / 90887
(4028, 0, 153998, 1, 4790, 17, 68453),
-- 4029 / 90886
(4029, 0, 154001, 1, 4790, 17, 68453),
(4029, 1, 154002, 1, 4790, 17, 68453);

DELETE FROM `quest_treasure_pickers` WHERE `QuestID` IN (90882, 90883, 90885, 90886, 90887);
INSERT INTO `quest_treasure_pickers` (`QuestID`, `TreasurePickerID`, `OrderIndex`) VALUES
(90882, 4025, 0),
(90883, 4026, 0),
(90885, 4027, 0),
(90886, 4029, 0),
(90887, 4028, 0);
