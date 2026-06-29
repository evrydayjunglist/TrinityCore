-- Gate 9 bootstrap: updater tables + minimal playerbots schema (TC-native, not WotLK bulk)

DROP TABLE IF EXISTS `updates`;
CREATE TABLE `updates` (
  `name` varchar(200) NOT NULL COMMENT 'filename with extension of the update.',
  `hash` char(40) DEFAULT '' COMMENT 'sha1 hash of the sql file.',
  `state` enum('RELEASED','ARCHIVED','CUSTOM') NOT NULL DEFAULT 'RELEASED',
  `timestamp` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `speed` int unsigned NOT NULL DEFAULT 0,
  PRIMARY KEY (`name`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

DROP TABLE IF EXISTS `updates_include`;
CREATE TABLE `updates_include` (
  `path` varchar(200) NOT NULL COMMENT 'directory to include. $ means relative to the module source directory.',
  `state` enum('RELEASED','ARCHIVED','CUSTOM') NOT NULL DEFAULT 'RELEASED',
  PRIMARY KEY (`path`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

INSERT INTO `updates_include` (`path`, `state`) VALUES
  ('$/data/sql/playerbots/updates', 'RELEASED');

DROP TABLE IF EXISTS `playerbots_db_version`;
CREATE TABLE `playerbots_db_version` (
  `version` int unsigned NOT NULL DEFAULT 1
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

INSERT INTO `playerbots_db_version` (`version`) VALUES (1);

DROP TABLE IF EXISTS `playerbots_account_keys`;
CREATE TABLE `playerbots_account_keys` (
  `account_id` int unsigned NOT NULL,
  `key_hash` varchar(64) NOT NULL,
  `created_at` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`account_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

DROP TABLE IF EXISTS `playerbots_account_links`;
CREATE TABLE `playerbots_account_links` (
  `owner_account_id` int unsigned NOT NULL,
  `linked_account_id` int unsigned NOT NULL,
  `created_at` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`owner_account_id`, `linked_account_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
