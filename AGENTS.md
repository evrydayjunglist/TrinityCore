# Agent Instructions — TrinityCore (Midnight)

**Full onboarding for new agents:** read [`docs/new-agent-intro.md`](docs/new-agent-intro.md) first.

## What this repository is

**TrinityCore `master`**: Midnight-era emulator fork. **Phase 1 complete** (2026-06-25):
empty AzerothCore-like `modules/` shell (`MODULES=none` default). Playerbots = gated
Phase 2+ under `modules/mod-playerbots/`. Default path = human Battle.net login.

Living status: `docs/midnight-assessment/current-state.md`

## Reference vs implementation

| Path | Role | Modify? | Build? |
|------|------|---------|--------|
| `modules/mod-playerbots/` | **Target** native Playerbots module (when built) | Yes, when tasked | Yes, once `MODULES` exists |
| `src/`, `cmake/`, `sql/updates/` | **Core (real)** | Yes, when tasked | Yes |
| `docs/midnight-assessment/` | Living assessment, gates, baseline | Yes, when tasked | N/A |
| `scripts/` | Build/helper scripts for this fork | Yes, when tasked | N/A |
| `BfaCore-Reforged/` | BfA fork — process, gates, **hosts reference trees** | **No** | **No** |
| `BfaCore-Reforged/mod-playerbots-master/` | Upstream AC mod-playerbots (WotLK) | **No** | **No** |
| `BfaCore-Reforged/azerothcore-wotlk-master/` | AC WotLK core reference | **No** | **No** |
| `BfaCore-Reforged/azerothcore-wotlk-Playerbot/` | AC WotLK Playerbot branch | **No** | **No** |

Canonical reference paths: `docs/midnight-assessment/reference-trees-and-standards.md`

Also treat root-level `azerothcore/`, `mod-playerbots/`, or similar checkouts as
**legacy read-only reference** if present — prefer `BfaCore-Reforged/` paths.

**Name trap:** `modules/mod-playerbots/` is the **implementation** path for this
fork; `BfaCore-Reforged/mod-playerbots-master/` is WotLK upstream reference. Confirm path before
every edit.

**Scripts name trap:** root `scripts/` = PowerShell/Python helpers;
`src/server/scripts/` = built-in C++ game scripts (`SCRIPTS` CMake option).

Reference trees: `.gitignore`d, do not commit, do not link or bulk-paste.
`BfaCore-Reforged/` (when present) holds additional core/process references.

## Direction

- **AzerothCore-like:** modular, config-driven, small gates, baseline protected
- **mod-playerbots-like:** real player bots/sessions — **no** WotLK wholesale import

**AzerothCore alignment is the default.** Module layout, config paths, CMake
surface, and runtime behavior should match AzerothCore (and BfaCore process where
applicable) unless TrinityCore constraints require otherwise. **If a change
deviates from that model — even for convenience — stop and review with the user
first** (options, trade-offs, then implement only after they choose). Do not
make executive layout or semantics decisions on your own.

Gate checklist: `docs/midnight-assessment/current-state.md`

## Baseline

After changes (unless user says otherwise): when a module system exists,
`MODULES=none` must build; human login must still work per
`docs/midnight-assessment/successful-local-baseline.md`.

## Key docs

- `docs/README.md` — fork vs upstream `doc/` (singular)
- `docs/new-agent-intro.md` — complete agent introduction
- `docs/midnight-assessment/current-state.md` — canonical living status
- `docs/midnight-assessment/successful-local-baseline.md` — build/run/login (to be verified)
- `docs/midnight-assessment/build-and-run-readiness.md` — build/run gates + module pitfalls
- `docs/midnight-assessment/module-support-prereq.md` — module guardrails (target design)
- `docs/midnight-assessment/playerbots-integration-plan.md` — Playerbots constraints and phases
- `scripts/build-trinitycore-master.ps1` — canonical local build script (auto-detects evry paths)
- `doc/` — upstream TrinityCore how-tos (`HowToScript.txt`, `UnixInstall.txt`, …)
- `docs/midnight-assessment/reference-trees-and-standards.md` — reference tree paths + AC/module standards
- `docs/midnight-assessment/sql-update-conventions.md` — SQL update naming + upstream sync
- `docs/midnight-assessment/dracthyr-forbidden-reach-handoff.md` — side project: Dracthyr intro overview; **§8 evidence/gap labels**
- `docs/midnight-assessment/dracthyr-phase-1b-handoff.md` — Phase 1b lower War Creche (mostly done; playtest bugs B1–B3)
- `docs/midnight-assessment/dracthyr-b4-azurathel-speak-handoff.md` — **active bugfix:** B4 Azurathel **181056** speak/turn-in blocker
- `docs/midnight-assessment/dracthyr-intro-opening-retail-parity-handoff.md` — **retail parity** R1–R3 (R2 accept user-validated 2026-06-26)
- `docs/midnight-assessment/dracthyr-intro-quest-accept-proper-fix-handoff.md` — **done:** Track B pending accept + `CLOSE_INTERACTION` popup grant (canonical)
- `docs/midnight-assessment/dracthyr-intro-quest-accept-fallback-removal-handoff.md` — **done:** Hack 2 (2s fallback) removed
- `modules/README.md` — product module layout

Prior-attempt gate/handoff docs are archived under
`docs/midnight-assessment/archive/pre-restart/` — not current state.
