-- TrinityCore-evry DB Rank 2 zero-model: legacy creature_template DELETE
-- Build 12.0.7.68275 — entries absent from Creature.db2 (owner-approved)
-- Evidence: creature_12_0_7_68275.csv
-- Distinct creature_template entries: 48
-- Label: retail-like / non-hacky

CREATE TEMPORARY TABLE `tmp_creature_zero_model_delete` (
  `entry` int unsigned NOT NULL,
  PRIMARY KEY (`entry`)
) ENGINE=Memory;

INSERT INTO `tmp_creature_zero_model_delete` (`entry`) VALUES
  (1823),
  (1825),
  (18604),
  (20267),
  (21634),
  (24692),
  (25537),
  (25544),
  (25683),
  (25704),
  (26080),
  (26340),
  (29229),
  (29243),
  (30426),
  (30427),
  (30428),
  (30913),
  (30926),
  (30938),
  (35531),
  (38168),
  (38258),
  (38726),
  (38736),
  (39120),
  (39121),
  (39122),
  (40022),
  (46450),
  (48968),
  (49070),
  (50194),
  (50195),
  (50196),
  (51088),
  (51467),
  (53988),
  (75917),
  (80581),
  (88133),
  (89833),
  (118153),
  (176813),
  (176845),
  (176849),
  (176850),
  (176851);

DELETE child FROM `creature_template_model` AS child
INNER JOIN `tmp_creature_zero_model_delete` AS d ON child.`CreatureID` = d.`entry`;
DELETE child FROM `creature_template_addon` AS child
INNER JOIN `tmp_creature_zero_model_delete` AS d ON child.`entry` = d.`entry`;
DELETE child FROM `creature_template_difficulty` AS child
INNER JOIN `tmp_creature_zero_model_delete` AS d ON child.`Entry` = d.`entry`;
DELETE child FROM `creature_template_gossip` AS child
INNER JOIN `tmp_creature_zero_model_delete` AS d ON child.`CreatureID` = d.`entry`;
DELETE child FROM `creature_template_locale` AS child
INNER JOIN `tmp_creature_zero_model_delete` AS d ON child.`entry` = d.`entry`;
DELETE child FROM `creature_template_movement` AS child
INNER JOIN `tmp_creature_zero_model_delete` AS d ON child.`CreatureId` = d.`entry`;
DELETE child FROM `creature_template_resistance` AS child
INNER JOIN `tmp_creature_zero_model_delete` AS d ON child.`CreatureID` = d.`entry`;
DELETE child FROM `creature_template_sparring` AS child
INNER JOIN `tmp_creature_zero_model_delete` AS d ON child.`Entry` = d.`entry`;
DELETE child FROM `creature_template_spell` AS child
INNER JOIN `tmp_creature_zero_model_delete` AS d ON child.`CreatureID` = d.`entry`;

DELETE ct FROM `creature_template` AS ct
INNER JOIN `tmp_creature_zero_model_delete` AS d ON ct.`entry` = d.`entry`;

DROP TEMPORARY TABLE `tmp_creature_zero_model_delete`;
