-- Rank 1 startup cleanup: stale Ring of Valor BG template + arena season 32 game event
-- R1-B: battleground_template id 11 removed from BattlemasterList.db2 on 12.0.x
DELETE FROM `battleground_template` WHERE `ID` = 11;

-- R1-A: Arena.ArenaSeason.ID defaults to 32 (World.cpp / worldserver.conf.dist); world DB had seasons through 11 only.
-- Retail note: startup stub for GameEventMgr::StartArenaSeason() world-DB lookup only — NOT verified against
-- PvpSeason.db2 for client 12.0.7.68275. Client SeasonInfo still comes from config. See handoff
-- docs/midnight-assessment/upstream-core-maintenance-rank1-handoff.md#retail-alignment before upstream PR.
SET @EENTRY := 96;

DELETE FROM `game_event` WHERE `eventEntry` = @EENTRY;
INSERT INTO `game_event` (`eventEntry`,`start_time`,`end_time`,`occurence`,`length`,`holiday`,`holidayStage`,`description`,`world_event`,`announce`) VALUES
(@EENTRY, NULL, NULL, 5184000, 2592000, 0, 0, 'PvP Season 32 - Midnight', 0, 2);

DELETE FROM `game_event_arena_seasons` WHERE `season` = 32;
INSERT INTO `game_event_arena_seasons` (`eventEntry`,`season`) VALUES
(@EENTRY, 32);
