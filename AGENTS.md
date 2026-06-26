# Agent Instructions — TrinityCore (Midnight)

**Full onboarding for new agents:** read [`docs/new-agent-intro.md`](docs/new-agent-intro.md) first.

## What this repository is

**TrinityCore `master`**: Midnight-era emulator fork with opt-in native
`modules/mod-playerbots`. Default path = normal human Battle.net login.
Playerbots = gated, disabled by default.

Living status: `docs/midnight-assessment/current-state.md`

## Reference vs implementation

| Path | Role | Modify? | Build? |
|------|------|---------|--------|
| `modules/mod-playerbots/` | **Native Playerbots module (real)** | Yes, when tasked | Yes, if `MODULES=static` |
| `src/`, `cmake/`, `sql/updates/` | **Core (real)** | Yes, when tasked | Yes |
| `docs/midnight-assessment/` | Living assessment, gates, baseline | Yes, when tasked | N/A |
| `scripts/` | Build/helper scripts for this fork | Yes, when tasked | N/A |
| `BfaCore-Reforged/` | BfA fork — process and gate **reference** | **No** | **No** |
| `mod-playerbots-master/` | Upstream AC mod-playerbots (WotLK) | **No** | **No** |
| `azerothcore-wotlk-master/` | AC WotLK core reference | **No** | **No** |
| `azerothcore-wotlk-Playerbot/` | AC WotLK Playerbot branch | **No** | **No** |

Also treat `azerothcore/`, `mod-playerbots/`, `azerothcore-playerbots/`, or any
similar root-level checkout as **read-only reference** — not this product.

**Name trap:** `modules/mod-playerbots/` is real; `mod-playerbots-master/` is
reference. Confirm path before every edit.

**Scripts name trap:** root `scripts/` = PowerShell/Python helpers;
`src/server/scripts/` = built-in C++ game scripts (`SCRIPTS` CMake option).

Reference trees: `.gitignore`d, do not commit, do not link or bulk-paste.

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

After changes (unless user says otherwise): `MODULES=none` builds; human login
still works per `docs/midnight-assessment/successful-local-baseline.md`.

## Key docs

- `docs/README.md` — fork vs upstream `doc/` (singular)
- `docs/new-agent-intro.md` — complete agent introduction
- `docs/midnight-assessment/step3-agent-handoff.md` — Step 3 / Gate 1 history (build done; live validation pending)
- `docs/midnight-assessment/playerbots-gate-01-compile-result.md` — Gate 1 result
- `docs/midnight-assessment/build-and-run-readiness.md` — build/run gates + module pitfalls
- `docs/midnight-assessment/step2-agent-handoff.md` — Step 1 → Step 2 history
- `doc/` — upstream TrinityCore how-tos (`HowToScript.txt`, `UnixInstall.txt`, …)
- `docs/midnight-assessment/module-support-prereq.md`
- `docs/midnight-assessment/playerbots-integration-plan.md`
- `modules/README.md`
- `scripts/build-trinitycore-master.ps1` — canonical local build script
