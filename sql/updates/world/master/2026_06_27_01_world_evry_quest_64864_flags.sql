-- Dracthyr intro 64864: retail 12.0.7 sniff has QuestFlags[2] = 8 (NOT_REPLAYABLE).
UPDATE `quest_template` SET `FlagsEx2` = `FlagsEx2` | 8 WHERE `ID` = 64864 AND (`FlagsEx2` & 8) = 0;
