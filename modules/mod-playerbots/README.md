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
    mod_playerbots_server_script.cpp # Bot outbound-packet (SMSG) observer -> signal layer
    mod_playerbots_world_script.cpp # Gate 9 world tick
  Bot/
    Factory/
      RandomPlayerbotFactory.h/.cpp # AC-shaped random-bot generation: account + level-1 char provisioning
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
      SignalTrigger.h/.cpp          # Packet-observation: consume-on-read externally-fired trigger
    Action/
      FollowAction.h/.cpp           # Gate 8 MoveFollow
      AttackAction.h/.cpp           # Gate 8 attack my target; Gate 8 follow-ups (B1) chase
      AttackAnythingAction.h/.cpp   # Gate 10b always-on kill role (random/newrpg bots)
      AttackValidity.h/.cpp         # Gate 10 shared hostility/LOS check (extracted from AttackAction)
      DeathActions.h/.cpp           # Bot death V1: release spirit / run to corpse / reclaim (corpse run)
      GroupActions.h/.cpp           # Gate 8 follow-ups (B2) master-alt auto-accept party invite
      WanderAction.h/.cpp           # Gate 10 idle random-offset movement (validated via SafeMovement)
      NewRpgBaseAction.h/.cpp       # Gate 10b shared RPG helpers (quest filters, POI resolve, MoveFarTo)
      NewRpgActions.h/.cpp          # Gate 10b status-update/go-grind/do-quest actions
      QuestGiverAction.h/.cpp       # Gate 10/10b generic nearby quest-giver accept/turn-in
      SafeMovement.h/.cpp           # Gate 10 ground-clip fix: validated-path + slope-checked moves
    Rpg/
      GrindLocationCache.h/.cpp     # Gate 10 Layer 1: lazy per-map SQL-scan grind spot cache
      HubLocationCache.h/.cpp       # RPG GO_CAMP: lazy per-map cache of NPC-hub (quest-giver cluster) centroids
      QuestItemDropCache.h/.cpp     # Convergence F4: lazy item id -> quest-loot drop-source creature entries
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

This module is part of the **validation baseline** — the project builds/boots and tests
human login with all modules enabled (`MODULES=static`). `MODULES=none` (the CMake default)
does not link this module and is kept only as a secondary "core still compiles" check.

> **Sticky cache gotcha (2026-07-04):** `-DMODULE_MOD_PLAYERBOTS` persists in the CMake
> cache. If a prior `MODULES=none` build left it `disabled`, a plain
> `.\scripts\build-trinitycore-master.ps1 -Modules static` will **not** re-enable it (the
> script doesn't pass the per-module flag), silently shipping a worldserver with the module
> compiled out while `mod-playerbots.conf` still loads. Pass `-DMODULE_MOD_PLAYERBOTS=static`
> explicitly (as above) or wipe the cache, and confirm the generated `ModulesLoader.cpp`
> calls `Addmod_playerbotsScripts()`.

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
### Random bot generation (RandomPlayerbotFactory — AC-shaped, TC-native)

When `Playerbots.Enable=1` and `MaxRandomBots>0`, startup auto-provisions reserved bot accounts and level-1 characters, then enumerates the roster from them (no hand-list). See [`playerbots-random-bot-generation-handoff.md`](../../docs/midnight-assessment/playerbots/playerbots-random-bot-generation-handoff.md).

- `Playerbots.RandomBotAccountPrefix` — default `rndbot`; bot accounts are named `<prefix><N>` and reserved via this prefix (third reserved-account mode). Empty disables generation + prefix reservation.
- `Playerbots.RandomBotAccountCount` — `0` = auto (`ceil(MaxRandomBots / CharactersPerAccount)`); a value below the minimum is warned and overridden
- `Playerbots.RandomBotRandomPassword` — `0` default; `1` gives generated accounts a random password
- `Playerbots.DeleteRandomBotAccounts` — **loud opt-in teardown**: `1` deletes all bot accounts/characters then stops the server; reset to `0` and restart to regenerate. Off by default
- `Playerbots.EnableAlliedRaces` — default `1`; when set, generated bots may roll **allied races** (Void Elf, Dark Iron, Vulpera, Earthen, ...), detected data-first via `ChrRacesFlag::IsAlliedRace` + expansion (no id list). `0` = base races only. See [`playerbots-special-races-classes-s1-allied-races-handoff.md`](../../docs/midnight-assessment/playerbots/playerbots-special-races-classes-s1-allied-races-handoff.md)
- `Playerbots.AllowedRaces` — optional curated include-list of race ids (comma-separated); empty (default) = every expansion-allowed race, non-empty restricts to exactly those ids (still masked by expansion / disabled-race / `EnableAlliedRaces`)
- `Playerbots.EnableHeroClasses` — default `1` (**Session 2**); when set, generated bots may roll **DK / DH / Evoker**, detected data-first by the `CLASS_*` enum (no id list), and the **Dracthyr** race (Evoker's paired race) is un-gated. `0` = base + allied classes only. See [`playerbots-special-races-classes-s2-hero-classes-handoff.md`](../../docs/midnight-assessment/playerbots/playerbots-special-races-classes-s2-hero-classes-handoff.md). **Note:** hero-class bots currently spawn in their instanced intro scenarios (Acherus / Mardum / Forbidden Reach); teaching bots to complete/exit those starting scenarios is future work
- `Playerbots.DisableDeathKnightLogin` — AC's `AiPlayerbot.DisableDeathKnightLogin` analog; default `0`. `1` excludes Death Knight even with `EnableHeroClasses` on (DH/Evoker unaffected)
- `Playerbots.AllowedClasses` — optional curated include-list of class ids (comma-separated); empty (default) = every allowed class, non-empty restricts to exactly those ids (still masked by expansion / disabled-class / hero-class toggles)
- Generated characters start at the **core-correct level** with a valid random race/class/gender + DB2-derived appearance — base races level 1, **allied races level 10** (`Player::Create` applies `CONFIG_START_ALLIED_RACE_LEVEL`), **hero classes** at their core start level (`CONFIG_START_{DEATH_KNIGHT,DEMON_HUNTER,EVOKER}_PLAYER_LEVEL`; no literal). Loadout/gear/talents remain out of scope
- Bot accounts are set to the realm **expansion** at provisioning (allied/special races unlocked) and are **Bnet-linked** at creation so bot sessions resolve a real `battlenetAccountId` (no `battlenet_*` FK growth)

**DEPRECATED** (superseded by generation + DB-enumerated roster; no longer read):

- `Playerbots.RandomBotAccounts` — was a comma-separated reserved account id list
- `Playerbots.RandomBotCharacterNames` — was a matching character-name list

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

**Full operator reference** (args, RBAC, constraints, quick-start):
[`docs/midnight-assessment/playerbots/playerbots-in-game-commands.md`](../../docs/midnight-assessment/playerbots/playerbots-in-game-commands.md).

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
  `GoCamp`/`DoQuest`/`WanderNpc`/`Rest`), reset whenever `newrpg` is (re)applied in `ResetStrategies()`
- `NewRpgStatusUpdateAction` — weighted status transitions (`Playerbots.RpgStatusProbWeight*`),
  duration-gated exits (`Playerbots.RpgStatusDuration*Ms`)
- `NewRpgGoCampAction` (`"new rpg go camp"`) — travels (`MoveFarTo`) to the nearest same-zone NPC
  hub from `Bot/Rpg/HubLocationCache` (a cluster of >= 3 quest givers, built from creature spawn
  data), then hands off to `WANDER_NPC` on arrival to mingle. Lets a bot stranded in a giver-less
  pocket reach a town and pick up new quests. Weight `Playerbots.RpgStatusProbWeight.GoCamp`.
- `NewRpgWanderNpcAction` (`"new rpg wander npc"`) — AC's lifelike hub mingling (`RPG_WANDER_NPC`):
  a grid scan (`Playerbots.RpgWanderNpcRadius`) picks the next NPC to visit — an actionable quest
  giver first (quest acquisition stays the priority; the always-on `QuestGiverAction` accepts within
  its 80yd radius), else a not-recently-visited allowed-flag hub NPC — walks up to it, dwells
  `Playerbots.RpgWanderNpcStayTime`, then cycles to the next. Un-strands bots whose log holds
  nothing actionable. Available when a giver or a >= 3-NPC hub is in range. Weight
  `Playerbots.RpgStatusProbWeight.WanderNpc`.
- `RPG_REST` — AC-parity timed sit (no trigger/action): the bot sits (`SetStandState(SIT)`) and
  returns to idle after `Playerbots.RpgStatusDuration.Rest`. Also the empty-availability fallback
  (sits instead of wandering aimlessly). Weight `Playerbots.RpgStatusProbWeight.Rest`.
- `NewRpgBaseAction::IsQuestUnactionable` — a COMPLETE quest with no ender anywhere in world data
  (e.g. junk `55660`) is added to an **in-memory, per-session** ignore set so quest selection stops
  treating it as pursuable — **never** `DropQuest`/DB-touch (owner directive), self-heals on relog
- `NewRpgDoQuestAction` — typed `QuestObjective` progress tracking (modern objective IDs, not
  WotLK fixed-array index math), `GetQuestPOIPosAndObjective` blob resolution, POI-stay timeout
  → abandon-set (`Playerbots.RpgPoiStayTimeMs`)
- `AttackAnythingAction` also kills **neutral** creatures a live quest objective wants dead via a
  data-first quest-kill target searcher (`AttackValidity::CollectQuestKillEntries`/
  `FindNearbyQuestKillTarget`, from the bot's own quest log) — `QUEST_OBJECTIVE_MONSTER` entries
  directly (e.g. mottled boars for "Cutting Teeth") and, since the convergence fixes,
  `QUEST_OBJECTIVE_ITEM` kill-and-loot drop sources via `Bot/Rpg/QuestItemDropCache` (e.g. scorpid
  workers for "Sting of the Scorpid"). Its always-on hostile search stays hostile-only, so bots
  never grief neutral wildlife they have no quest for. `Playerbots.RpgQuestKillSearchRadius`
- `AttackAnythingAction` keeps the combat chase **owning movement for the whole fight**: `IsUseful`
  stays useful while the bot holds a valid victim in combat, and `Execute`'s sustain branch keeps
  that victim, faces it, and re-asserts `MoveChase` (only when the active generator isn't already
  `CHASE`, via the unchanged `IsApproachPathWalkable` slope gate). Without it, once the bot has a
  victim the action bowed out and an RPG-travel `MovePoint` clobbered the chase, parking the bot
  ~5–6 yd from a *ranged* attacker (e.g. Northwatch Scout `39317`, quest `25172`) it never closed on
  — a melee mob masked this by closing itself. Player-like "stay on the mob," no combat-power buff,
  no rotation/kiting; see
  [`playerbots-rpg-combat-ranged-attacker-engagement-handoff.md`](../../docs/midnight-assessment/playerbots/playerbots-rpg-combat-ranged-attacker-engagement-handoff.md).
- `NewRpgBaseAction::MoveFarTo` — travel on top of `SafeMovement`'s validated-path contract, with
  stuck-detection teleport recovery (depends on the bot-session teleport self-ack fix); short and
  far legs share one progress-guarded attempt + leg-scaled forward-cone fallback (see *Bot obstacle
  pathing* below)
- `OrganizeQuestLog` — drops greyed-out/incapable/failed quests when the log is nearly full,
  same 3-pass AC contract
- `.playerbot status` surfaces each bot's live RPG status string + quest accept/reward/complete/
  abandon/drop counters (`NewRpgStatistic`, labelled *this session*) — fork replacement for AC's
  whisper-based `TellRpgStatusAction`. Filtered to one bot (`.playerbot status <name>` or target a
  bot) it additionally dumps identity + the real quest log with per-objective progress and
  `[ignored]`/`[low-prio]` flags, so questing state is grounded in facts, not a session counter

Closeout: [`playerbots-gate-10b-do-quest-handoff.md`](../../docs/midnight-assessment/playerbots/playerbots-gate-10b-do-quest-handoff.md).

## Bot death handling (V1 — the corpse run)

A dead random/newrpg bot now brings itself back instead of lying dead forever. `Bot/Action/DeathActions`
adds three death-state-gated actions to `NewRpgStrategy`'s always-on band **above** the interact band
(relevance `release spirit` 55 > `run to corpse` 54 > `reclaim corpse` 53 > `quest giver` 30). Each is
`IsUseful()` only in its matching death phase, so exactly one is live at a time and only while the bot is
dead — a live bot is completely unaffected. Mirrors AC's `DeadStrategy` → `ReleaseSpiritAction` /
`ReviveFromCorpseAction` **shape**, but is TC-native and **packetless**: instead of AC's synthetic
`CMSG_REPOP_REQUEST` / `CMSG_RECLAIM_CORPSE`, each action replicates the guard set of the modern core
handler (`WorldSession::HandleRepopRequest` / `HandleReclaimCorpse`) and calls the public `Player`
methods those thin handlers wrap — same precedent as `TalkToQuestNpcAction` / `LootAction`.

- `ReleaseSpiritAction` (`"release spirit"`) — dead, not yet a ghost, no `SPELL_AURA_PREVENT_RESURRECTION`:
  after a lifelike `Playerbots.RpgDeathReleaseDelaySeconds` pause it runs the `HandleRepopRequest` body
  (`RemovePet` → `BuildPlayerRepop` → `RepopAtGraveyard` — the graveyard is the core's own data-first pick).
- `RunToCorpseAction` (`"run to corpse"`) — released ghost whose corpse is beyond the reclaim radius:
  walks back to `Player::GetCorpseLocation()` via the `SafeMovement` validated-path contract (slope/path
  gate honoured). Bounded stranding fallback: if it can't walk back within
  `Playerbots.RpgDeathCorpseRunTimeoutSeconds` it teleports to the corpse once (the only non-walking path).
- `ReclaimCorpseAction` (`"reclaim corpse"`) — released ghost on its corpse past the core reclaim delay:
  replicates every `HandleReclaimCorpse` guard (incl. `GetCorpseReclaimDelay`, so it never spams during
  the ~30s window) then `ResurrectPlayer(0.5f)` + `SpawnCorpseBones()`. `.playerbot status` shows
  per-session `deaths` / `revived` counters.

Solo random bots only. Master-alt bots run `follow`/`attack` (never `newrpg`), so the death band can't reach
them, and every action additionally short-circuits on `HasMaster()`. Spirit-healer res (penalties),
other-bot/player res, self-res (soulstone/reincarnation), death-count teleport and master-alt/group death
are all **NYI** — see
[`playerbots-bot-death-corpse-run-handoff.md`](../../docs/midnight-assessment/playerbots/playerbots-bot-death-corpse-run-handoff.md).

## RPG quest-loop convergence fixes (F1–F4)

Four fixes that make the RPG loop reliably *converge* on questing (2026-07-08 Valley of Trials
playtest diagnosis — giver ping-pong + stealth-quest stalls):

- **F1 — strict quest-giver targeting (AC parity).** `QuestGiverAction`'s candidate search now
  gates each giver on the strict `HasQuestToAcceptOrReward` (transactable right now: rewardable
  COMPLETE, or takeable+worth+capable NONE) instead of "quest menu non-empty". TC menus include
  the bot's own INCOMPLETE turn-ins, so an ender of a quest the bot was still working anchored the
  action permanently useful while nothing could ever transact — hijacking WANDER_NPC/REST into a
  movement tug-of-war around the giver (the "milling around Gornek" crowd).
- **F2 — POI-area sweep.** Parked at an objective POI with zero progress, a bot now walks to a
  fresh random-weighted interior point of the same objective blob every
  `Playerbots.RpgPoiSweepIntervalSeconds` (default 25) instead of circling one fixed point — so it
  actually covers large objective areas and *discovers* stealthed objective mobs the retail-like
  way (stealth detection untouched; the `RpgPoiStayTime` abandon budget keeps running).
- **F3 — reachable kill targets.** `FindNearbyQuestKillTarget` prefers the nearest candidate whose
  approach passes the `SafeMovement` walkability probe (max 5 probed, plain-nearest fallback) so a
  bot doesn't latch onto a ridge-perched mob it can attack but never chase.
- **F4 — kill-and-loot quests.** `CollectQuestKillEntries` also arms `QUEST_OBJECTIVE_ITEM`
  objectives by mapping the item to its quest-loot drop-source creatures
  (`Bot/Rpg/QuestItemDropCache`, a lazy one-shot reverse index over the ObjectMgr
  `creature_questitem` store). Gameobject-gathered items map to no creatures and stay with the
  `UseQuestObjectAction` gather path.

Handoff: [`playerbots-rpg-quest-convergence-fixes-handoff.md`](../../docs/midnight-assessment/playerbots/playerbots-rpg-quest-convergence-fixes-handoff.md).

## Bot obstacle pathing (D0–D2)

Extends `MoveFarTo`'s AC-shaped obstacle convergence — commit-validated-partial-progress +
forward-cone stepping-stone resampling — from the far (> 70 yd) branch down to **short legs**,
where local obstacles (e.g. the Valley of Trials Den bonfire between Kaltunk and Gornek) actually
sit. Every fix routes *around* an obstacle by finding a better **validated** move; the ⭐
standing-watch `SafeMovement` slope/path-type contract is untouched (never relaxed).

- **D0 — rejection instrumentation.** `SafeMovement` logs (debug-only, `playerbots` logger) which
  gate refuses a move — path-type, slope, or low-progress `PATHFIND_INCOMPLETE` — with the leg
  length, so a playtest can *see* which mechanism fires at an obstacle instead of guessing.
- **D1 — short-leg progress guard.** `MoveFarTo`'s short branch used to accept a zero-progress
  `PATHFIND_INCOMPLETE` route (endpoint at the bot's own feet) as success, burning the tick
  standing still. It now requires a leg-scaled minimum gain (1 yd on short approach legs, AC's 5 yd
  on far legs) and falls through on failure.
- **D2 — leg-scaled cone fallback.** A failed short leg now reaches the same forward-cone
  stepping-stone sampler the far branch uses (2 samples, ±π/2, each fully SafeMovement-validated),
  with the sample distance scaled to the leg (`min(PATH_FINDER_DIS, disToDest)`) so an ~18 yd
  obstacle leg samples ~9–18 yd stones, not 35–70 yd overshoots — far-branch behavior unchanged.
  `QuestGiverAction`'s giver approach now routes through `MoveFarTo` too (was the one raw,
  fallback-less locomotion call site), inheriting D1/D2 and the shared stuck accounting. WANDER_NPC
  / DO_QUEST callers already used `MoveFarTo`, so their blind 360° `MoveRandomNear` nudge is now the
  last tier behind a directed recovery.

Handoff: [`playerbots-bot-obstacle-pathing-handoff.md`](../../docs/midnight-assessment/playerbots/playerbots-bot-obstacle-pathing-handoff.md);
extends the ⭐ standing-watch [`playerbots-bot-wander-ground-clip-handoff.md`](../../docs/midnight-assessment/playerbots/playerbots-bot-wander-ground-clip-handoff.md).

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
Remaining known AC-likeness gaps (per-class rotations, talent/gear/security/text subsystems)
are tracked, not silently dropped — see
[`playerbots-future-gates-roadmap.md` § Beyond Gate 10](../../docs/midnight-assessment/playerbots/playerbots-future-gates-roadmap.md#beyond-gate-10-not-numbered-here).

## Bot outbound-packet observation (SMSG signal layer)

Bot sessions are socketless, so every packet the server would send them is dropped in
`WorldSession::SendPacket`'s bot branch. That branch now first fires TC's **existing**
`ServerScript::OnPacketSend` hook — the fork's single `src/` change for this feature (one added
line, the same hook real-player sends already fire; `IsBotSession()`-scoped, no human-path change,
inert under `MODULES=none`). `mod_playerbots_server_script` observes that hook and routes bot
packets to `BotPlayerbotAI::HandleBotOutgoingPacket`, which mirrors AC's `botOutgoingPacketHandlers`
shape: a static **opcode → trigger-name** registry (`LookupPacketSignal` in `BotPlayerbotAI.cpp`)
whose hits are pushed onto a bounded, mutex-guarded per-bot queue. The queue is swapped out on the
bot's own AI tick into a per-tick fired set, and a consume-on-read `SignalTrigger` turns a delivered
signal into a normal engine trigger — the triggered *action* then reads live server state through
public core APIs.

**THE design rule — opcode-as-signal ONLY; never parse packet payloads.** A captured packet is a
wake-up signal keyed by its opcode and nothing else — no `packet >>` decode anywhere in module code.
AC can parse payloads because WotLK 3.3.5's wire format is frozen; this fork tracks Midnight, where
wire formats churn every build, so payload parsing would reintroduce exactly the standing merge
liability the socketless/packetless design exists to avoid. Any future feature that genuinely needs
payload data is a **separate owner decision**, not a drive-by extension. Because a signal can be
lost (bounded queue, logout race), every signal-driven feature must also stay correct if the signal
never arrives.

Threading: `OnPacketSend` runs on arbitrary map/World sender threads, so the observer does nothing
but a registry test + guarded enqueue; **all reaction happens single-threaded on the bot's tick.**

V1 registers **one** entry — `SMSG_PARTY_INVITE → "group invite signal"` — wired to the existing
master-alt `accept invitation` action. The original `GetGroupInvite()` poll is kept as a fallback
(accept is idempotent, so double-handling is benign). Master toggle: `Playerbots.PacketObservation.Enable`
(default on). Full design: [`playerbots-bot-packet-observation-handoff.md`](../../docs/midnight-assessment/playerbots/playerbots-bot-packet-observation-handoff.md).
