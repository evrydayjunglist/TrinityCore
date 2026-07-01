-- Dracthyr intro retail-parity fix (R1 camera pan), from 12.0.7.68275 sniff
-- dump_12.0.7.68275_2026-06-30_15-11-47 (begin login -> Awaken, Dracthyr complete):
--
-- 1) SMSG_TRIGGER_MOVIE CinematicID=969 fires at the very start of login, ~0.2s
--    before the first SMSG_PLAY_SCENE. 2026_06_26_02 cleared `intro_movie_id`
--    on the (wrong) assumption that movie 969 conflicted with the room-movie
--    scene pipeline; retail plays both. This is almost certainly the missing
--    "camera pans from above the building down to the player" effect (R1).
--    Core already auto-fires this via CharacterHandler.cpp (`SendMovieStart`)
--    when `playercreateinfo.intro_movie_id` is set and `!getCinematic()` —
--    no C++ change needed, only the DB column.
--
-- 2) Spell 365560 (Stasis / SPELL_STASIS_4, the player freeze) is cast in the
--    same instant as 97709 (Altered Form), well before the room teleport
--    (~2s earlier than the teleport in the sniff). Restore it to
--    `playercreateinfo_cast_spell` (matches pre-fork upstream
--    2023_01_02_00_world.sql) so the player is frozen from the very first
--    tick, not only after `RunDracthyrLoginIntro` teleports them. The C++
--    cast in `RunDracthyrLoginIntro` stays as-is — it is still required for
--    the 369744 quest-abandon restart path, which does not go through
--    `playercreateinfo_cast_spell`.
--
-- See docs/midnight-assessment/dracthyr/dracthyr-intro-opening-retail-parity-handoff.md.

UPDATE `playercreateinfo` SET `intro_movie_id` = 969 WHERE `race` IN (52, 70) AND `class` = 13;

DELETE FROM `playercreateinfo_cast_spell` WHERE `raceMask` = 98304 AND `classMask` = 4096 AND `spell` = 365560;
INSERT INTO `playercreateinfo_cast_spell` (`raceMask`, `classMask`, `createMode`, `spell`, `note`) VALUES
(98304, 4096, 0, 365560, 'Dracthyr - Evoker - Stasis (freeze; matches retail login timing, before room teleport)');
