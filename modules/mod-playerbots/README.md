# mod-playerbots (Gates 1–10)

Playerbots module for this TrinityCore fork. **Disabled by default.**

**Primary-track motto:** **mod-playerbots north star** — *AC-shaped stepping stones;
TC-native implementation.*
[`project-focus.md`](../../docs/midnight-assessment/project-focus.md#mod-playerbots-north-star-primary-track)

**North star detail:** Stepping stones toward **AC mod-playerbots** architecture
(read-only on disk under `BfaCore-Reforged/mod-playerbots-master/` — **gitignored**;
use `Test-Path` + Read on absolute paths, not workspace search alone —
[`reference-trees-on-disk-alert.md`](../../docs/midnight-assessment/reference-trees-on-disk-alert.md)) — see
`docs/midnight-assessment/playerbots/playerbots-integration-plan.md` § AC north star
and `playerbots-future-gates-roadmap.md`.

**Locked contract:** [`playerbots-ac-contract-handoff.md`](../../docs/midnight-assessment/playerbots/playerbots-ac-contract-handoff.md)

## Module layout (AC-aligned)

```text
modules/mod-playerbots/src/
  Script/
    mod_playerbots_loader.cpp
    PlayerbotCommandScript.cpp    # .playerbot commands
    mod_playerbots_player_script.cpp
    mod_playerbots_world_script.cpp # Gate 9 world tick
  Bot/
    Engine/                         # Gate 6 Engine skeleton
      Engine.h/.cpp
      AiObjectContext.h/.cpp
      AiFactory.h/.cpp
      PlayerbotAIAware.h
      Action.h/.cpp, Trigger.h, Strategy.h, Event.h
      PlayerbotAIBase.h/.cpp
    Strategy/
      PassiveStrategy.h/.cpp        # Gate 6 passive (GM bots without master)
      FollowMasterStrategy.h/.cpp   # Gate 8 follow; Gate 8 follow-ups (B2) group-invite trigger
      CombatStrategy.h/.cpp         # Gate 8 attack
      NewRpgStrategy.h/.cpp         # Gate 10/10b world RPG loop (masterless random bots)
    Trigger/
      GroupTriggers.h/.cpp          # Gate 8 follow-ups (B2) master group-invite poll
      NewRpgTriggers.h/.cpp         # Gate 10b RPG status-machine triggers
    Action/
      FollowAction.h/.cpp           # Gate 8 MoveFollow
      AttackAction.h/.cpp           # Gate 8 attack my target; Gate 8 follow-ups (B1) chase
      AttackAnythingAction.h/.cpp   # Gate 10b always-on kill role (random/newrpg bots)
      AttackValidity.h/.cpp         # Gate 10 shared hostility/LOS check (extracted from AttackAction)
      GroupActions.h/.cpp           # Gate 8 follow-ups (B2) master-alt auto-accept party invite
      WanderAction.h/.cpp           # Gate 10 idle random-offset movement (validated via SafeMovement)
      NewRpgBaseAction.h/.cpp       # Gate 10b shared RPG helpers (quest filters, POI resolve, MoveFarTo)
      NewRpgActions.h/.cpp          # Gate 10b status-update/go-grind/do-quest actions
      QuestGiverAction.h/.cpp       # Gate 10/10b generic nearby quest-giver accept/turn-in
      SafeMovement.h/.cpp           # Gate 10 ground-clip fix: validated-path + slope-checked moves
    Rpg/
      GrindLocationCache.h/.cpp     # Gate 10 Layer 1: lazy per-map SQL-scan grind spot cache
      NewRpgInfo.h/.cpp             # Gate 10b per-bot RPG state machine + statistics
    BotPlayerbotAI.h/.cpp           # Gate 5–10b bot AI
  Mgr/
    BotSessionMgr.h/.cpp            # TC socketless sessions (Gates 3–4, 7, 9 scheduler)
    PlayerbotMgr.h/.cpp             # Per-human master-alt control (Gate 7)
    PlayerbotsMgr.h/.cpp            # AI registry (Gate 5) + mgr map (Gate 7)
    RandomPlayerbotMgr.h/.cpp       # Random bot scheduler (Gate 9); periodic DB save (C1)
  Db/
    PlayerbotsDatabaseMgr.h/.cpp    # Playerbots DB version check (Gate 9)
  data/sql/playerbots/              # Module-local SQL (Gate 9)
    base/playerbots_gate9_base.sql
    updates/
  Playerbots.h                      # GET_PLAYERBOT_AI / GET_PLAYERBOT_MGR macros
  PlayerbotsConfig.h                # AC analogue: PlayerbotAIConfig.h at src root
  conf/mod-playerbots.conf.dist
```

## Build (opt-in)

```powershell
.\scripts\build-trinitycore-master.ps1 -Modules static
cmake -B "D:/WOWEmulation/Emulators/Builds/TrinityCore-evry" `
      -DMODULES=static -DMODULE_MOD_PLAYERBOTS=static
cmake --build "D:/WOWEmulation/Emulators/Builds/TrinityCore-evry" `
      --config RelWithDebInfo --target worldserver modules
```

Default path (`MODULES=none`) does not link this module.

## Runtime config

- Shipped default: `bin/RelWithDebInfo/modules/mod-playerbots.conf.dist`
- User override: `bin/RelWithDebInfo/modules/mod-playerbots.conf`

Loaded via `ConfigMgr::LoadModuleConfigDir("modules")` — separate from `worldserver.conf.d`.

Key options: `Playerbots.Enable`, `Playerbots.ReservedAccount.Ids` (or MinId/MaxId),
`Playerbots.MaxActiveBots`, `Playerbots.AllowAccountBots`, `Playerbots.MaxAddedBots`,
`Playerbots.ReactDelay`, `Playerbots.GlobalCooldown`, `Playerbots.FollowDistance`, `Playerbots.LogLevel`.
Full current list (movement/RPG/save tuning included) lives in
[`conf/mod-playerbots.conf.dist`](conf/mod-playerbots.conf.dist) — not duplicated key-by-key here
to avoid this section drifting out of sync with the shipped defaults.

**Gate 9 — Playerbots database + random bots:**

- `PlayerbotsDatabaseInfo` — MySQL connection string (e.g. `127.0.0.1;3306;trinity;trinity;tc_playerbots`)
- `Playerbots.Updates.EnableDatabases` — `0` default; set `1` to run module SQL updater on startup
- `PlayerbotsDatabase.WorkerThreads` / `PlayerbotsDatabase.SynchThreads`
- `Playerbots.MinRandomBots` / `Playerbots.MaxRandomBots` — default `0` (feature off when `MaxRandomBots=0`)
- `Playerbots.RandomBotAutologin` — scheduler autologin when `1`
- `Playerbots.DisabledWithoutRealPlayer` — default `1`; no random bots when no humans online
- `Playerbots.RandomBotAccounts` — comma-separated reserved account ids; **repeat an id to add another character on the same account** (AC parity — see [`playerbots-bot-session-account-cap-handoff.md`](../../docs/midnight-assessment/playerbots/playerbots-bot-session-account-cap-handoff.md))
- `Playerbots.RandomBotCharacterNames` — comma-separated character names, same order/index as `RandomBotAccounts` (one name per roster slot, not per account)

## AC contract (do not violate)

### Bot AI model

| Use | Do not use |
|-----|------------|
| `PlayerbotsMgr` + `GET_PLAYERBOT_AI` | `Player::SetAI` / `UnitAI` for bots |
| `PlayerScript::OnPlayerAfterUpdate` → `UpdateAI` | TC charmed-player AI stack |
| `Player::IsBot()` (optional TC convenience) | — |

Core enforces: `Unit::SetAI` **refuses** players on `IsBotSession()` sessions.

### Command surface (AC reference: `PlayerbotCommandScript.cpp`)

Top-level command is **`.playerbot`** (singular).

| Subcommand | AC analogue | This fork |
|------------|-------------|-----------|
| `bot` | Master-alt control (`PlayerbotMgr`) | `.playerbot bot add/remove/list/logout` (Gate 7) |
| `rndbot` | Random bot GM ops | `.playerbot rndbot status/start/stop` (Gate 9) |
| `account setKey/link/linkedAccounts/unlink` | Account linking | NYI stubs |
| `login` / `logout` | — | **TC extension** — GM socketless bots on reserved accounts |
| `status` | — | **TC extension** — enabled, count/max, policy, active bot names |

### Reserved account policy (TC adaptation)

Dedicated reserved `auth.account` ids for bot characters — see contract handoff.

## Gate 3–4 (login / logout)

When `Playerbots.Enable = 1` and reserved accounts are configured:

- `.playerbot login <characterName>` — socketless bot on reserved account (GM)
- `.playerbot logout [characterName]` — save and remove bot session (GM)

## Gate 5 (AI registry shell)

- `PlayerbotsMgr` registers `BotPlayerbotAI` on bot login; removes on logout
- `OnPlayerAfterUpdate` ticks AI shell
- `Playerbots.MaxActiveBots` caps concurrent bot sessions
- `.playerbot status` shows active count/max, policy, and bot character names

Closeout: [`playerbots-gate-05-inert-ai-shell-handoff.md`](../../docs/midnight-assessment/playerbots/playerbots-gate-05-inert-ai-shell-handoff.md) — **complete** (owner playtest 2026-06-29).

## Gate 6 (Engine + passive strategy)

- `Engine::DoNextAction` runs on `BotPlayerbotAI::UpdateAIInternal` (AC-shaped tick)
- `AiObjectContext` registers strategies and actions
- `AiFactory` reads class/spec from Midnight `Player` / DB2
- `ResetStrategies()` — GM bots without master get `+passive` only
- Config: `Playerbots.ReactDelay`, `Playerbots.GlobalCooldown`
- `Playerbots.LogLevel >= 1` — throttled engine debug (`playerbots` filter)

Closeout: [`playerbots-gate-06-engine-skeleton-handoff.md`](../../docs/midnight-assessment/playerbots/playerbots-gate-06-engine-skeleton-handoff.md) — **complete** (agent 2026-06-29; owner playtest pending).

## Gate 7 (master–alt control)

- `PlayerbotMgr` per human on login (`AddPlayerbotData(player, false)`)
- `.playerbot bot add/remove/list/logout` — same-account alts only (`AllowAccountBots`)
- `MaxAddedBots` per master + `MaxActiveBots` global cap
- `BotSessionMgr::LoginMasterAlt` — socketless login without reserved-account requirement
- Core: `World::AddBotSession` — GUID-keyed bot session map, human master + one or more bot alts share one `accountId` (see [`playerbots-bot-session-account-cap-handoff.md`](../../docs/midnight-assessment/playerbots/playerbots-bot-session-account-cap-handoff.md))

Closeout: [`playerbots-gate-07-master-alt-handoff.md`](../../docs/midnight-assessment/playerbots/playerbots-gate-07-master-alt-handoff.md) — **complete** (agent 2026-06-29; owner playtest pending).

## Gate 8 (movement + combat MVP)

- Master-alt bots: `follow` + `attack` strategies via `ResetStrategies()` after `SetMaster`
- `FollowAction` → `MotionMaster::MoveFollow(master, FollowDistance)` when out of range
- `AttackMyTargetAction` → attacks `master->GetTarget()` via `Unit::Attack` (melee auto-attack),
  then `MotionMaster::MoveChase` to close and stay on the target (**B1 follow-up**, 2026-07-03 —
  originally attacked in place)
- Single-engine relevance routing (`attack` 10.0 > `follow` 1.0); no dual `BotState` yet
- GM `.playerbot login` bots without master stay **passive**
- Config: `Playerbots.FollowDistance` (default 3.0)
- **Party/group join (B2 follow-up, 2026-07-03):** a master-alt bot auto-accepts a pending
  invite from its own master — `GroupTriggers::GroupInviteTrigger` polls `Player::GetGroupInvite()`
  (socketless bots never see AC's `SMSG_GROUP_INVITE`) and `GroupActions::AcceptInvitationAction`
  accepts via `WorldSession::HandlePartyInviteResponseOpcode`. Random-bot/any-player accept stays
  **NYI** (AC gates it behind `PlayerbotSecurity`, itself not built here).

Closeout: [`playerbots-gate-08-movement-combat-handoff.md`](../../docs/midnight-assessment/playerbots/playerbots-gate-08-movement-combat-handoff.md) — **complete** (agent 2026-06-29; owner playtest pending); follow-ups: [`playerbots-master-alt-followups-handoff.md`](../../docs/midnight-assessment/playerbots/playerbots-master-alt-followups-handoff.md).

## Gate 9 (Playerbots database + random bot bootstrap)

- **Option A** core hook: `PlayerbotsDatabase` pool when built with `-DMODULE_MOD_PLAYERBOTS=static` (`WITH_PLAYERBOTS`)
- Module SQL under `data/sql/playerbots/` — version table + optional account key/link tables
- `PlayerbotsDatabaseMgr` — connection/version check from module
- `RandomPlayerbotMgr` — world-tick scheduler (5s); logs in reserved-account bots via `BotSessionMgr::LoginReservedCharacter`
- Random bots: **passive** AI by default (no master); Gate 10 adds an opt-in `newrpg` AI — see below; `DisabledWithoutRealPlayer` stops scheduler when last human logs out
- **Roster:** pair `RandomBotAccounts` with `RandomBotCharacterNames` by index; multiple entries can
  share the same account id (AC parity — several random-bot characters on one reserved account
  in-world together, e.g. account 3 → `Three`, account 3 → `Threethree`) — fixed
  2026-07-03, see [`playerbots-bot-session-account-cap-handoff.md`](../../docs/midnight-assessment/playerbots/playerbots-bot-session-account-cap-handoff.md)
- `.playerbot rndbot status|start|stop` — GM ops surface
- `.playerbot account setKey/link` — remain NYI (schema exists empty)

**Owner setup:**

1. `CREATE DATABASE tc_playerbots;`
2. Set `PlayerbotsDatabaseInfo` + `Playerbots.Updates.EnableDatabases = 1` in `modules/mod-playerbots.conf`
3. `worldserver -u` or restart with updates enabled
4. For autologin smoke test: `MaxRandomBots = 1`, `RandomBotAutologin = 1`, roster on **account 3+** (not Robot's account 2)
5. Raise `MaxActiveBots` if testing GM Robot + random bot together

Closeout: [`playerbots-gate-09-db-random-bootstrap-handoff.md`](../../docs/midnight-assessment/playerbots/playerbots-gate-09-db-random-bootstrap-handoff.md) — **complete** (agent + owner playtest 2026-06-29).

## Gate 10 (World RPG loop)

Random bots start at their **normal** per-race/faction character-creation position — no
forced teleport, no owner-picked test zone — then optionally run a generic wander/grind/
quest-giver loop driven entirely by the world DB, matching AC mod-playerbots'
`NewRpgStrategy` shape.

**Level config (AC `PlayerbotAIConfig.h` analogues):**

- `Playerbots.RandomBotMinLevel` / `Playerbots.RandomBotMaxLevel` — inclusive first-login
  level roll range (`MaxLevel = 0` → server max level)
- `Playerbots.DisableRandomLevels` + `Playerbots.RandombotStartingLevel` — fixed starting
  level instead of a roll
- `Playerbots.RandomBotFixedLevel` — AC semantics: a toggle, not a level; `1` locks the bot
  at its current level (no XP gain at all)
- `Playerbots.RandomBotXPRate` — XP multiplier for random bots (ignored when `FixedLevel = 1`)
- Wired via `mod_playerbots_player_script`: `OnLogin` sets the starting level once (level 1
  only, so re-logins don't re-roll); `OnGiveXP` applies the fixed-level/rate behavior

**World RPG loop — two-layer grind-spot design (matches AC's real split, not a single
mechanism):**

- **Layer 1 (`GrindLocationCache`, "where to grind"):** lazy per-map cache built from
  `sObjectMgr->GetAllCreatureData()` (in-memory creature spawns, not a fresh SQL query),
  filtered to hostile/attackable/lootable creatures with resolvable level data
  (`ContentTuningID` → `ContentTuning.db2`, or `DeltaLevelMin/Max` fallback), grid-clustered
  into ~50yd cells. Built once per map on first use; no live invalidation (Layer 2 always
  re-validates against live state).
- **Layer 2 (live grid search, "what to attack/interact with right now"):** standard TC
  grid searches over currently-loaded `Creature`/`GameObject`s near the bot's actual
  position — reuses the Gate 8 hostility/LOS check (extracted into
  `Bot/Action/AttackValidity.h`) for combat targets, and `Player::PrepareQuestMenu` +
  `PlayerTalkClass->GetQuestMenu()` for quest givers.
- `NewRpgStrategy` default actions (relevance): `quest giver` (30.0) > `attack anything` (20.0)
  > `new rpg status update` (11.0), with the status machine's own `go grind`/`wander`/`do quest`
  handlers at 3.0 — a bot always checks for quest opportunities and fights back before
  progressing its current RPG status. Superseded by Gate 10b's full status machine (below);
  see `NewRpgStrategy.cpp` for the up-to-date relevance layout.
- `Playerbots.RandomBotRpgChance` (0–100) — chance a masterless random bot runs `newrpg`
  instead of `passive`, rolled in `BotPlayerbotAI::ResetStrategies()`. Fork-native knob: AC's
  own `randomBotRpgChance` picks between RPG-vs-grind sub-states inside an already-RPG bot;
  here the RPG subset itself is the only active behavior, so this is the single
  on/off-with-a-dial knob for it.
- GM `.playerbot login` bots and master-alt bots are unaffected — the `newrpg` branch only
  applies when `!HasMaster() && sRandomPlayerbotMgr->IsRandomBot(guid)`.

Closeout: [`playerbots-gate-10-world-rpg-slice-handoff.md`](../../docs/midnight-assessment/playerbots/playerbots-gate-10-world-rpg-slice-handoff.md).

## Gate 10b (RPG_DO_QUEST — goal-directed questing)

Masterless random bots running `newrpg` can now pick a quest from their own log, travel to
the objective's POI, make kill/collect progress, walk back, and turn it in — not just
opportunistic nearby pickup.

- `Bot/Rpg/NewRpgInfo` — per-bot `std::variant` state machine (`Idle`/`WanderRandom`/`GoGrind`/
  `DoQuest`), reset whenever `newrpg` is (re)applied in `ResetStrategies()`
- `NewRpgStatusUpdateAction` — weighted status transitions (`Playerbots.RpgStatusProbWeight*`),
  duration-gated exits (`Playerbots.RpgStatusDuration*Ms`)
- `NewRpgDoQuestAction` — typed `QuestObjective` progress tracking (modern objective IDs, not
  WotLK fixed-array index math), `GetQuestPOIPosAndObjective` blob resolution, POI-stay timeout
  → abandon-set (`Playerbots.RpgPoiStayTimeMs`)
- `NewRpgBaseAction::MoveFarTo` — long-distance travel on top of `SafeMovement`'s validated-path
  contract, with stuck-detection teleport recovery (depends on the bot-session teleport self-ack
  fix)
- `OrganizeQuestLog` — drops greyed-out/incapable/failed quests when the log is nearly full,
  same 3-pass AC contract
- `.playerbot status` surfaces each bot's live RPG status string + quest accept/reward/complete/
  abandon/drop counters (`NewRpgStatistic`) — fork replacement for AC's whisper-based
  `TellRpgStatusAction`

Closeout: [`playerbots-gate-10b-do-quest-handoff.md`](../../docs/midnight-assessment/playerbots/playerbots-gate-10b-do-quest-handoff.md).

## Quest loot + object interaction

RPG bots pick up quest drops and use quest-objective gameobjects while questing/grinding. Both go
straight through the core's own APIs — **packetless** (a socketless bot session has no inbound
packet queue) — and lean on the core for every ownership/anti-ninja rule.

- `Bot/Action/LootAction` (`"loot"`) — AC's `OpenLootAction`/`StoreLootAction` shape, TC-native:
  live grid scan for the nearest dead creature the bot `Player::isAllowedToLoot`s that holds a
  wanted item, `Player::SendLoot` to open it, then `Player::StoreLootItem` for each slot that
  `LootItem::GetUiTypeForPlayer` says is takeable. It also drains any already-open loot window
  (`Player::GetAELootView`), which is how loot from a gameobject the object-use action opened gets
  stored. Default filter is quest-relevant items only (AC's `IsLootAllowed` quest core:
  `ItemTemplate::GetStartQuest` + incomplete `QUEST_OBJECTIVE_ITEM` match) — `Playerbots.LootDistance`,
  `Playerbots.LootQuestItemsOnly`, `Playerbots.LootMoney`.
- `Bot/Action/UseQuestObjectAction` (`"use quest object"`) — finds a spawned GO matching an
  incomplete `QUEST_OBJECTIVE_GAMEOBJECT` in the bot's log (`GO_STATE_READY`, not
  `GO_FLAG_INTERACT_COND`/`GO_FLAG_NOT_SELECTABLE`), approaches via the `SafeMovement` contract, then
  replays the client's exact gate (`Player::GetGameObjectIfCanInteractWith` → `GameObject::Use`).
  `GameObject::Use` fires the real quest credit and, for chest/gathering objectives, opens the loot
  window that `LootAction` then drains — `Playerbots.QuestObjectUseDistance`.
- **GO turn-in** needs no new code: TC quest enders are always `GAMEOBJECT_TYPE_QUESTGIVER`, which
  the existing `QuestGiverAction` already handles (`CanInteractWithQuestGiver`/`PrepareQuestMenu`
  both accept gameobjects).
- Relevance (in the always-on interact band, all combat-gated via `IsUseful`): `quest giver` (30) >
  `use quest object` (25) > `loot` (22) > `attack anything` (20) > `new rpg status update` (11).
- `.playerbot status` adds `looted`/`objects used` counters (`NewRpgStatistic`).

Closeout: [`playerbots-quest-loot-and-object-interaction-handoff.md`](../../docs/midnight-assessment/playerbots/playerbots-quest-loot-and-object-interaction-handoff.md).

## Review follow-ups (2026-07-03)

Owner-directed AC-likeness review of `track/playerbots` found and closed three small gaps
beyond the numbered gates — **B1** (Gate 8 combat-chase), **B2** (Gate 8 party/group join,
folded into the Gate 8 section above), and **C1/C2** (periodic random-bot DB save +
engine-selection documentation note, see the `RandomPlayerbotMgr`/`Engine` bullets above).
Full detail: [`playerbots-master-alt-followups-handoff.md`](../../docs/midnight-assessment/playerbots/playerbots-master-alt-followups-handoff.md),
[`playerbots-review-c-followups-handoff.md`](../../docs/midnight-assessment/playerbots/playerbots-review-c-followups-handoff.md).
Remaining known AC-likeness gaps (per-class rotations, talent/gear/security/text subsystems,
bot outbound-packet observation) are tracked, not silently dropped — see
[`playerbots-future-gates-roadmap.md` § Beyond Gate 10](../../docs/midnight-assessment/playerbots/playerbots-future-gates-roadmap.md#beyond-gate-10-not-numbered-here).
