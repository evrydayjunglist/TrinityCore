-- Dracthyr Forbidden Reach (B5 correction): a deeper re-pass of the retail sniff found Tethalash
-- does NOT stay vanished from the Lower War Creche while a separately pre-spawned "186864" double
-- waits in the back room (owner-rejected on playtest: "I dont think the dracthyr downstairs is
-- supposed just vanish", 2026-06-30). She's smoothly re-entried in place (181680 -> 186864, same
-- technique conceptually as Azurathel's UpdateEntry) and then walks the whole way there via 7
-- plain ground-steered spline legs (see zone_the_forbidden_reach.cpp
-- DracthyrTethalashWakeEvent / DRACTHYR_TETHALASH_REUNITED_TARGET for the extracted waypoints).
-- 186864 is now applied to the SAME creature object at runtime via `UpdateEntry`, not spawned as
-- a separate permanent NPC — remove the static spawn added by 2026_06_30_05.
--
-- See docs/midnight-assessment/dracthyr/dracthyr-forbidden-reach-handoff.md §B5/§11a-2.

DELETE FROM `creature` WHERE `guid`=9003905 AND `id`=186864;

-- creature_text row for CreatureID 186864 (added by 2026_06_30_05) stays valid unchanged — it's
-- still keyed on the same entry, just now reached via UpdateEntry instead of a static spawn.
