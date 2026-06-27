-- Dracthyr intro: skip generic intro movie 969; room movie spells (394245–394282) handle the camera pan.

UPDATE `playercreateinfo` SET `intro_movie_id` = NULL WHERE `race` IN (52, 70) AND `class` = 13;
