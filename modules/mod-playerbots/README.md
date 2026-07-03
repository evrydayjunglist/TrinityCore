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
      Action.h, Trigger.h, Strategy.h, Event.h
      PlayerbotAIBase.h/.cpp
    Strategy/
      PassiveStrategy.h/.cpp        # Gate 6 passive (GM bots without master)
      FollowMasterStrategy.h/.cpp   # Gate 8 follow
      CombatStrategy.h/.cpp         # Gate 8 attack
      NewRpgStrategy.h/.cpp         # Gate 10 world RPG loop (masterless random bots)
    Action/
      FollowAction.h/.cpp           # Gate 8 MoveFollow
      AttackAction.h/.cpp           # Gate 8 attack my target
      AttackValidity.h/.cpp         # Gate 10 shared hostility/LOS check (extracted from AttackAction)
      WanderAction.h/.cpp           # Gate 10 idle random-offset movement
      GrindAction.h/.cpp            # Gate 10 cached grind spot + live hostile search/attack
      QuestGiverAction.h/.cpp       # Gate 10 generic nearby quest-giver accept/turn-in
    Rpg/
      GrindLocationCache.h/.cpp     # Gate 10 Layer 1: lazy per-map SQL-scan grind spot cache
    BotPlayerbotAI.h/.cpp           # Gate 5–10 bot AI
  Mgr/
    BotSessionMgr.h/.cpp            # TC socketless sessions (Gates 3–4, 7, 9 scheduler)
    PlayerbotMgr.h/.cpp             # Per-human master-alt control (Gate 7)
    PlayerbotsMgr.h/.cpp            # AI registry (Gate 5) + mgr map (Gate 7)
    RandomPlayerbotMgr.h/.cpp       # Random bot scheduler (Gate 9)
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
- `AttackMyTargetAction` → attacks `master->GetTarget()` via `Unit::Attack` (melee auto-attack)
- Single-engine relevance routing (`attack` 10.0 > `follow` 1.0); no dual `BotState` yet
- GM `.playerbot login` bots without master stay **passive**
- Config: `Playerbots.FollowDistance` (default 3.0)

Closeout: [`playerbots-gate-08-movement-combat-handoff.md`](../../docs/midnight-assessment/playerbots/playerbots-gate-08-movement-combat-handoff.md) — **complete** (agent 2026-06-29; owner playtest pending).

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
- `NewRpgStrategy` (`STRATEGY_TYPE_NONCOMBAT`) — `quest giver` (30.0) > `grind` (20.0) >
  `wander` (10.0) default actions; a bot always checks for quest opportunities before
  grinding, and grinds before idly wandering.
- `Playerbots.RandomBotRpgChance` (0–100) — chance a masterless random bot runs `newrpg`
  instead of `passive`, rolled in `BotPlayerbotAI::ResetStrategies()`. Fork-native knob: AC's
  own `randomBotRpgChance` picks between RPG-vs-grind sub-states inside an already-RPG bot;
  here the RPG subset itself is the only active behavior, so this is the single
  on/off-with-a-dial knob for it.
- GM `.playerbot login` bots and master-alt bots are unaffected — the `newrpg` branch only
  applies when `!HasMaster() && sRandomPlayerbotMgr->IsRandomBot(guid)`.

Closeout: [`playerbots-gate-10-world-rpg-slice-handoff.md`](../../docs/midnight-assessment/playerbots/playerbots-gate-10-world-rpg-slice-handoff.md).
