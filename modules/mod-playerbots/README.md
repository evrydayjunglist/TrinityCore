# mod-playerbots (Gates 1–7)

Playerbots module for this TrinityCore fork. **Disabled by default.**

**Primary-track motto:** **mod-playerbots north star** — *AC-shaped stepping stones;
TC-native implementation.*
[`project-focus.md`](../../docs/midnight-assessment/project-focus.md#mod-playerbots-north-star-primary-track)

**North star detail:** Stepping stones toward **AC mod-playerbots** architecture
(`D:\WOWEmulation\Emulators\Source\TrinityCore-evry\BfaCore-Reforged\mod-playerbots-master\`, read-only) — see
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
  Bot/
    Engine/                         # Gate 6 Engine skeleton
      Engine.h/.cpp
      AiObjectContext.h/.cpp
      AiFactory.h/.cpp
      PlayerbotAIAware.h
      Action.h, Trigger.h, Strategy.h, Event.h
      PlayerbotAIBase.h/.cpp
    Strategy/
      PassiveStrategy.h/.cpp        # Gate 6 passive only
    BotPlayerbotAI.h/.cpp           # Gate 5–6 bot AI
  Mgr/
    BotSessionMgr.h/.cpp            # TC socketless sessions (Gates 3–4, 7)
    PlayerbotMgr.h/.cpp             # Per-human master-alt control (Gate 7)
    PlayerbotsMgr.h/.cpp            # AI registry (Gate 5) + mgr map (Gate 7)
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
`Playerbots.ReactDelay`, `Playerbots.GlobalCooldown`, `Playerbots.LogLevel`.

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
| `rndbot` | Random bot GM ops | NYI stub (Gate 9) |
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
- `AiObjectContext` stub registers **passive** strategy only
- `AiFactory` reads class/spec from Midnight `Player` / DB2
- `ResetStrategies()` on bot AI construct — `+passive` only; **no movement**
- Config: `Playerbots.ReactDelay`, `Playerbots.GlobalCooldown`
- `Playerbots.LogLevel >= 1` — throttled engine debug (`playerbots` filter)

Closeout: [`playerbots-gate-06-engine-skeleton-handoff.md`](../../docs/midnight-assessment/playerbots/playerbots-gate-06-engine-skeleton-handoff.md) — **complete** (agent 2026-06-29; owner playtest pending).

## Gate 7 (master–alt control)

- `PlayerbotMgr` per human on login (`AddPlayerbotData(player, false)`)
- `.playerbot bot add/remove/list/logout` — same-account alts only (`AllowAccountBots`)
- `MaxAddedBots` per master + `MaxActiveBots` global cap
- `BotSessionMgr::LoginMasterAlt` — socketless login without reserved-account requirement
- Core: `World::AddBotSession` — human + bot on same `accountId`

Closeout: [`playerbots-gate-07-master-alt-handoff.md`](../../docs/midnight-assessment/playerbots/playerbots-gate-07-master-alt-handoff.md) — **complete** (agent 2026-06-29; owner playtest pending).

## Gate 8 (next)

Movement + combat MVP — [`playerbots-future-gates-roadmap.md`](../../docs/midnight-assessment/playerbots/playerbots-future-gates-roadmap.md) § Gate 8.
