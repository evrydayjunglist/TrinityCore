ALTER TABLE `updates`
    MODIFY COLUMN `state` ENUM('RELEASED', 'ARCHIVED', 'MODULE')
    CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT 'RELEASED'
    COMMENT 'Defines if an update is released, archived, or supplied by a module.';
