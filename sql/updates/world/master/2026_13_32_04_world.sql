-- Arathi RPE Phase 3 polish: farm Thrall 244656 was faction 2057 (rank1 DB
-- maintenance), which is hostile to Horde. Retail sniff H CreateObject at Go'shek
-- Farm shows FactionTemplate 35 for Bruvk 244729, farm Thrall 244656, and farm
-- Jaina 244655 (owner playtest Huntee Horde: Bruvk/Jaina OK, Thrall red+aggro).

UPDATE `creature_template`
SET `faction`=35,
    `VerifiedBuild`=68453
WHERE `entry`=244656
  AND `faction`<>35;
