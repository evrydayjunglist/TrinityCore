-- Dracthyr Forbidden Reach: personal guide (Kodethi/Dervishian) wake-up Conversation.
--
-- Owner report: after breaking the guide out of stasis (spellclick 362355), the guide "just
-- stands there" on this fork, whereas on retail it talks and then walks off. Retail 12.0.7.68275
-- sniff (Horde/Kodethi, Room 3) shows a 2-line Conversation object created right after the
-- disintegrate hit:
--   Line 48297 (BroadcastText 221640, ActorIndex 1): "Wha... what... $p! What has happened?
--     Who did this to us?!"
--   Line 48298 (BroadcastText 223150, ActorIndex 1, StartTime +8948ms): "Something is very
--     wrong here. We must find our wing mates. This way!"
-- Both ConversationLine rows (48297/48298) already exist in base client ConversationLine.db2 —
-- only the server-side conversation_template row is fork content; no conversation_actors row is
-- needed since the speaking actor is a dynamically-spawned per-player creature, added at runtime
-- via Conversation::AddActor (see spell_disintegrate_dracthyr_awaken /
-- StartDracthyrGuideWakeConversation in zone_the_forbidden_reach.cpp).
--
-- See docs/midnight-assessment/dracthyr/dracthyr-forbidden-reach-handoff.md.

DELETE FROM `conversation_line_template` WHERE `Id` IN (48297, 48298);
INSERT INTO `conversation_line_template` (`Id`, `UiCameraID`, `ActorIdx`, `Flags`, `ChatType`, `VerifiedBuild`) VALUES
(48297, 0, 1, 0, 0, 68275),
(48298, 0, 1, 0, 0, 68275);

DELETE FROM `conversation_template` WHERE `Id` = 18934;
INSERT INTO `conversation_template` (`Id`, `FirstLineId`, `TextureKitId`, `Flags`, `ScriptName`, `VerifiedBuild`) VALUES
(18934, 48297, 0, 0, '', 68275);
