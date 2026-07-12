--
-- Archaeology Survey fidelity: persist each active site's current hidden find so relog/restart
-- cannot relocate it. Fresh databases receive these columns from 2026_07_11_00_characters.sql;
-- this guarded migration upgrades databases that already applied that update.
--
SET @has_find_x := (
  SELECT COUNT(*)
  FROM `information_schema`.`COLUMNS`
  WHERE `TABLE_SCHEMA` = DATABASE()
    AND `TABLE_NAME` = 'character_research_site'
    AND `COLUMN_NAME` = 'findX'
);
SET @add_find_x := IF(
  @has_find_x = 0,
  'ALTER TABLE `character_research_site` ADD COLUMN `findX` float NOT NULL DEFAULT ''0'' COMMENT ''Current hidden find world X'' AFTER `progress`',
  'DO 0'
);
PREPARE `archaeology_add_find_x` FROM @add_find_x;
EXECUTE `archaeology_add_find_x`;
DEALLOCATE PREPARE `archaeology_add_find_x`;

SET @has_find_y := (
  SELECT COUNT(*)
  FROM `information_schema`.`COLUMNS`
  WHERE `TABLE_SCHEMA` = DATABASE()
    AND `TABLE_NAME` = 'character_research_site'
    AND `COLUMN_NAME` = 'findY'
);
SET @add_find_y := IF(
  @has_find_y = 0,
  'ALTER TABLE `character_research_site` ADD COLUMN `findY` float NOT NULL DEFAULT ''0'' COMMENT ''Current hidden find world Y'' AFTER `findX`',
  'DO 0'
);
PREPARE `archaeology_add_find_y` FROM @add_find_y;
EXECUTE `archaeology_add_find_y`;
DEALLOCATE PREPARE `archaeology_add_find_y`;
