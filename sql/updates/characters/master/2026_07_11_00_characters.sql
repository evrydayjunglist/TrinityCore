--
-- Archaeology (from the ground up) Phase 1 sub-slice 2b: per-character active dig sites.
-- Persists the ActivePlayer ResearchSites / ResearchSiteProgress update fields across relog/restart.
--
CREATE TABLE IF NOT EXISTS `character_research_site` (
  `guid` bigint unsigned NOT NULL DEFAULT '0' COMMENT 'Global Unique Identifier',
  `researchSiteId` smallint unsigned NOT NULL DEFAULT '0',
  `progress` int unsigned NOT NULL DEFAULT '0',
  `findX` float NOT NULL DEFAULT '0' COMMENT 'Current hidden find world X',
  `findY` float NOT NULL DEFAULT '0' COMMENT 'Current hidden find world Y',
  PRIMARY KEY (`guid`,`researchSiteId`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='Archaeology active dig sites per character';
