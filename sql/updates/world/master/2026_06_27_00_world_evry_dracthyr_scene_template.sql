-- Dracthyr intro: post-movie scene hook (idempotent; live DB verified 2026-06-27).
-- SceneId 3064, ScriptPackageID 3730 — chains stasis after room movie in C++ scene_dracthyr_evoker_intro.

DELETE FROM `scene_template` WHERE `SceneId` = 3064;
INSERT INTO `scene_template` (`SceneId`, `Flags`, `ScriptPackageID`, `Encrypted`, `ScriptName`) VALUES
(3064, 25, 3730, 0, 'scene_dracthyr_evoker_intro');
