# Agent Instructions — TrinityCore (Midnight)

**Full onboarding for new agents:** read [`docs/new-agent-intro.md`](docs/new-agent-intro.md) first.

**Fork steering (canonical):** [`docs/midnight-assessment/project-focus.md`](docs/midnight-assessment/project-focus.md)

## Local DB credentials (agents — use for all MySQL queries)

| Account | User | Password | DB names |
|---------|------|----------|----------|
| Root / admin | `root` | `123slam` | `world` `auth` `characters` `hotfixes` |
| App user | `trinity` | `trinity` | runtime only (worldserver/bnetserver) |

MySQL CLI: `D:/WOWEmulation/Emulators/Builds/TrinityCore-evry/bin/RelWithDebInfo/mysql/bin/mysql.exe`

Full details + query template: [`successful-local-baseline.md` § DB credentials](docs/midnight-assessment/successful-local-baseline.md#db-credentials-local-machine--agents-use-these-for-queries)

## What this repository is

**TrinityCore `master` fork** (Midnight era): tracks upstream TC `master` while adding
fork-specific work. This is **not** a vanilla TC checkout — it is a personal/special-interest
fork with its own gates, modules, and content goals.

Living status: `docs/midnight-assessment/current-state.md`

## Work tracks

Three tracks coexist — **follow the user's current task.** Full detail:
[`docs/midnight-assessment/project-focus.md`](docs/midnight-assessment/project-focus.md).

| Track | One line |
|-------|----------|
| **Upstream sync** | Stay current with TC `master` |
| **Primary** | AC-like `modules/` + native gated `mod-playerbots` (Gate 5 complete; Gate 6 next) |
| **Alternate** | Retail-parity content (Dracthyr intro today) |

Default path for all tracks: human Battle.net login on `MODULES=none`.

**Checkout policy (2026-06-30):** exactly **one project** may be checked out at a
time — the owner is one person and works one project at a time. Before starting
substantive work (code/SQL/doc edits), read and update
[`track-checkout-board.md`](docs/midnight-assessment/track-checkout-board.md); check
it back in before starting a different project. This supersedes any older "multiple
retail-parity projects in parallel" language elsewhere in the docs.

**Fix quality (fork mottos):** **Non-hacky by default**, **We don't break things**,
**See something, log something** (notice something broken/hacky/off-goal while doing other
work? document it, don't drive-by fix it), **AC-minded, TC-safe**, and **choose least hacky,
most retail-like** (decision tie-breaker when options compete) — retail/sniff/DB2 evidence,
minimal portable diffs, baseline protected; AC-like module discipline on native TrinityCore.
On the **primary track**
(`modules/`, `modules/mod-playerbots/`, Playerbots gates), also **mod-playerbots north star** —
*AC-shaped stepping stones; TC-native implementation.* See
[`project-focus.md` § mod-playerbots north star](docs/midnight-assessment/project-focus.md#mod-playerbots-north-star-primary-track).
**NYI**
only for genuine future feature work —
[`project-focus.md` § NYI vocabulary](docs/midnight-assessment/project-focus.md#nyi-vocabulary--the-only-defer-reason)
(**finish now unless NYI**).
Full bar: [`project-focus.md` § Fork mottos](docs/midnight-assessment/project-focus.md#fork-mottos).
Scoped tasks: [`agent-task-handoff-template.md`](docs/midnight-assessment/agent-task-handoff-template.md).

**Git commits (fork):** agents **do not** run `git commit` unless the owner explicitly
asks. Finish validation + closeout first; owner reviews and commits when ready.

**SQL (fork):** new tables/data → **`sql/updates/{world,auth,characters,hotfixes}/master/`**
only — **do not patch `sql/base/`** (incl. `sql/base/dev/`) unless the owner explicitly
requests a full base refresh. **Never hand-edit a base dump's `updates` table** (that's
TrinityCore's own official-release tracking) — no exception, even during a base refresh.
Existing dev DBs apply changes via **`worldserver -u`**. See
[`sql-update-conventions.md`](docs/midnight-assessment/sql-update-conventions.md).

## Reference vs implementation

> **AGENT ALERT:** `BfaCore-Reforged/` is **gitignored** — workspace search often finds **nothing** even when trees **exist on disk**. Do **not** conclude they are missing or default to GitHub. Run `Test-Path` / `Get-ChildItem` on the full paths below, then Read with absolute paths. Details: [`docs/midnight-assessment/reference-trees-on-disk-alert.md`](docs/midnight-assessment/reference-trees-on-disk-alert.md).

**`BfaCore-Reforged/` hosts read-only reference trees** (gitignored, **on disk** on the owner's machine).
Use them for AC-likeness and mod-playerbots-likeness — **never edit, build, link, or commit.**

| Path | Role | Modify? | Build? |
|------|------|---------|--------|
| `BfaCore-Reforged/azerothcore-wotlk-master/` | AC WotLK core — modules, CMake, loader | **No** | **No** |
| `BfaCore-Reforged/azerothcore-wotlk-Playerbot/` | AC WotLK core + Playerbot branch | **No** | **No** |
| `BfaCore-Reforged/mod-playerbots-master/` | Upstream mod-playerbots (WotLK) — **north star** | **No** | **No** |
| `BfaCore-Reforged/` (root) | BfA fork process/gates reference | **No** | **No** |

**Implementation (this fork — edit when tasked):**

| Path | Role | Modify? | Build? |
|------|------|---------|--------|
| `modules/mod-playerbots/` | Native Playerbots module | Yes | Yes (opt-in) |
| `src/`, `cmake/`, `sql/updates/` | TrinityCore core | Yes | Yes |
| `docs/midnight-assessment/` | Living assessment, gates | Yes | N/A |
| `scripts/` | Build/helper scripts | Yes | N/A |

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

**Primary (special interest):**

- **AzerothCore-like:** modular, config-driven, small gates, baseline protected; **core
  changes OK** for Playerbots when non-hacky and aligned —
  [`playerbots-integration-plan.md` § Core changes standard](docs/midnight-assessment/playerbots/playerbots-integration-plan.md#core-changes-src--development-standard)
- **mod-playerbots-like:** real player bots/sessions — **no** WotLK wholesale import;
  **mod-playerbots north star** on this track — *AC-shaped stepping stones; TC-native implementation.*
  See [`project-focus.md` § mod-playerbots north star](docs/midnight-assessment/project-focus.md#mod-playerbots-north-star-primary-track)

**Alternate (retail parity):**

- Script/content work that matches live retail behavior where TC `master` is thin
- Scoped diffs in core scripts and SQL; handoff docs per feature area
- Does not change module layout or Playerbots gates unless the user explicitly ties them together

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

**Local paths (this checkout):** [`successful-local-baseline.md`](docs/midnight-assessment/successful-local-baseline.md) § Local machine paths.
**Live retail sniff:** [`retail-packet-sniff-workflow.md`](docs/midnight-assessment/retail-packet-sniff-workflow.md) + `scripts/retail-sniff-capture.ps1`.

## Key docs

- `docs/midnight-assessment/project-focus.md` — **canonical fork steering** (work tracks, routing)
- `docs/README.md` — fork vs upstream `doc/` (singular)
- `docs/new-agent-intro.md` — complete agent introduction
- `docs/midnight-assessment/current-state.md` — canonical living status
- `docs/midnight-assessment/track-checkout-board.md` — **check before starting work** — which single project is checked out right now
- `docs/midnight-assessment/agent-prompt-templates.md` — **owner-facing** — copy/paste prompt starters for opening an agent chat, by task type
- `docs/midnight-assessment/fork-journal.md` — infra, upstream merges, repo hygiene chronology
- `docs/midnight-assessment/successful-local-baseline.md` — build/run/login (confirmed 2026-06-25; re-verify after upstream merges)
- `docs/midnight-assessment/build-and-run-readiness.md` — build/run gates + module pitfalls
- `docs/midnight-assessment/upstream-core-maintenance/upstream-core-maintenance.md` — server log triage; upstream drift + ROI fix backlog; **retail-ready by default** (owner approval for non-retail stubs); **agent validation warnings** (grep ≠ gameplay, `GetAura`, port **8085**)
- `docs/midnight-assessment/upstream-core-maintenance/upstream-core-maintenance-talent-reset-popup-handoff.md` — ad-hoc playtest fix: spurious talent reset popup on login/level-up (**complete** build 2026-06-28; owner playtest pending)
- `docs/midnight-assessment/upstream-db-maintenance.md` — **DBErrors.log** triage (world SQL drift); **delete log before fresh capture** (appends by default); ROI rank **1 partial** (batch closed), ranks **2–12** open; parser `parse-dberrors-log-inventory.py`
- `docs/midnight-assessment/upstream-db-maintenance-rank1-handoff.md` — DB Rank 1 creature faction (**partial — batch closed**; ~60k tail **NYI**)
- `docs/midnight-assessment/upstream-core-maintenance/upstream-core-maintenance-rank1-handoff.md` — Rank 1 startup quick wins (**complete**)
- `docs/midnight-assessment/upstream-core-maintenance/upstream-core-maintenance-rank2-handoff.md` — Rank 2 guild challenge re-enqueue (**complete** 2026-06-28)
- `docs/midnight-assessment/upstream-core-maintenance/upstream-core-maintenance-rank2b-handoff.md` — Rank 2b garrison / splash / empower re-enqueue (**complete** 2026-06-28)
- `docs/midnight-assessment/upstream-core-maintenance/upstream-core-maintenance-rank2c-handoff.md` — Rank 2c catalog / quick-join re-enqueue (**complete** 2026-06-28; Capture J validated)
- `docs/midnight-assessment/upstream-core-maintenance/upstream-core-maintenance-rank2d-handoff.md` — Rank 2d account-UI login re-enqueue batch (**complete** 2026-06-28; Capture L validated)
- `docs/midnight-assessment/upstream-core-maintenance/upstream-core-maintenance-r1a-r2c2-retail-parity-handoff.md` — **R1-A** `PvpSeason.db2` + **R2e/R2d C2** SMSG timing (**complete** 2026-06-28; Capture O/P validated)
- `docs/midnight-assessment/upstream-core-maintenance/upstream-core-maintenance-r1a-tierc-handoff.md` — **R1-A Tier C** `SMSG_SEASON_INFO` + DB2 season (**complete** 2026-06-28; owner login pass)
- `docs/midnight-assessment/upstream-core-maintenance/upstream-core-maintenance-r1a-tierc-f1-displayseason-gaps-handoff.md` — **R1-A Tier C F1** DisplaySeason store + gap fields **117** / **1069** (**complete** 2026-06-28)
- `docs/midnight-assessment/upstream-core-maintenance/upstream-core-maintenance-rank2e-handoff.md` — Rank 2e login re-enqueue tail (**complete** 2026-06-28; Capture N validated)
- `docs/midnight-assessment/upstream-core-maintenance/upstream-core-maintenance-rank3-rank4-handoff.md` — Rank 3–4 batch spell scripts (**complete** 2026-06-28)
- `docs/midnight-assessment/upstream-core-maintenance/upstream-core-maintenance-rank5-handoff.md` — Rank 5 cascading missing spell ids (**complete** 2026-06-28; R5-A playtest + GetAura follow-up)
- `docs/midnight-assessment/upstream-core-maintenance/upstream-core-maintenance-rank7-handoff.md` — Rank 7 warband groups CMSG wire parse (**complete** 2026-06-28; C2/C3 owner-validated)
- `docs/midnight-assessment/upstream-core-maintenance/upstream-core-maintenance-rank8-handoff.md` — Rank 8 unhandled opcode stubs (**complete** 2026-06-28; **19/19** wired; owner-validated **25 → 0**)
- `docs/midnight-assessment/upstream-core-maintenance/upstream-core-maintenance-rank8-f2-f5-sniff-upstream-handoff.md` — **R8-F2** stub SMSG sniff (**complete** 2026-06-28)
- `docs/midnight-assessment/housing/housing-r8-f4-get-player-houses-info-handoff.md` — **R8-F4** housing login stub (**complete** 2026-06-28; owner R8-F4 pass)
- `docs/midnight-assessment/upstream-core-maintenance/upstream-core-maintenance-rank9-handoff.md` — Rank 9 DBC mismatch batch (**complete** 2026-06-28; **138 → 0**; **no follow-ups**; § Phase 3B/3C planning; Warnings 7–9 in parent spec). **Next optional upstream ROI:** rank **10** or **11** — parent spec ROI table.
- `docs/midnight-assessment/upstream-core-maintenance/upstream-core-maintenance-rank2-retail-sniff-2026-06-27.md` — Rank 2 retail packet evidence (Captures C/D live retail; not evry realm)
- `docs/midnight-assessment/module-support-prereq.md` — module guardrails (target design)
- `docs/midnight-assessment/playerbots/playerbots-integration-plan.md` — Playerbots constraints, phases, § **AC north star**
- `scripts/build-trinitycore-master.ps1` — canonical local build script (auto-detects evry paths)
- `scripts/README.md` — fork helper script inventory (build, log triage, validators, sniff)
- `scripts/validate-rank5-deaths-embrace.ps1` — Rank 5 R5-A guard (code + log grep + SpellEffect layout)
- `doc/` — upstream TrinityCore how-tos (`HowToScript.txt`, `UnixInstall.txt`, …)
- `docs/midnight-assessment/reference-trees-and-standards.md` — reference tree paths + AC/module standards
- `docs/midnight-assessment/reference-trees-on-disk-alert.md` — **gitignored trees are on disk** — verify before claiming missing
- `docs/midnight-assessment/sql-update-conventions.md` — SQL update naming + upstream sync
- `docs/midnight-assessment/retail-data-first-checklist.md` — **before hardcoding client ids:** grep DB2, map packet fields, self-review
- `docs/midnight-assessment/db2-export-agent-instructions.md` — **agents:** EvryDb2Export / wago DB2 evidence before spell scripting
- `docs/midnight-assessment/db2-export-workflow.md` — DB2 CSV export workflow (owner + agents)
- `tools/README.md` — local WoWDBDefs + EvryDb2Export (DBCD) setup
- `docs/midnight-assessment/retail-packet-sniff-workflow.md` — **live retail** Ymir + WowPacketParser agent/owner loop
- `docs/midnight-assessment/agent-task-handoff-template.md` — **paste/read first** for scoped agent tasks (rules, sworn acknowledgment, task brief, closeout)
- **Cursor skills (gitignored — on disk locally, workspace search may find nothing):** under `D:\WOWEmulation\Emulators\Source\TrinityCore-evry\.cursor\skills\` — `trinitycore-db2-export` (DB2 export), `trinitycore-retail-sniff` (live retail sniff), `trinitycore-gm-commands` (GM playtest), `trinitycore-agent-task-handoff` (scoped task ritual + NYI labels), `trinitycore-retail-data-first` (pre-hardcoding checklist), `trinitycore-track-switch` (checkout/pause/resume — see `track-checkout-board.md`). `Test-Path` + Read with absolute path before claiming missing.
- **Cursor rules (gitignored, always/glob-scoped — not visible in a normal doc index):** under `.cursor\rules\` — see each `.mdc` frontmatter for scope; `track-checkout-required.mdc` (always-on, checkout board) and `owner-reports-are-ground-truth.mdc` / `reference-trees-gitignored.mdc` (always-on) are the highest-value ones to know exist.
- `docs/midnight-assessment/warbands/char-select-campsites-handoff.md` — char-select campsites (**complete** user 2026-06-27; not in-game housing)
- `docs/midnight-assessment/warbands/warbands-overall-handoff.md` — **warbands overall** (Phases 1 + 1.5 + **2 complete** user 2026-06-27; Phase 3–4 next)
- `docs/midnight-assessment/warbands/warbands-phase2-account-wide-currency-handoff.md` — Phase 2 account-wide currency **complete** (implementation spec + R1–R3 follow-ups)
- `docs/midnight-assessment/warbands/warbands-phase2plus-account-wide-earning-handoff.md` — Phase 2+ account-wide earning eligibility (deferred)
- `docs/midnight-assessment/warbands/warbands-renown-stub-handoff.md` — **Renown stub** (Adventure Guide UI + quest gates; deferred — backend currency done)
- `docs/midnight-assessment/warbands/warbands-phase3-warband-reputation-handoff.md` — Phase 3 warband reputation (**next** implementation spec)
- `docs/midnight-assessment/warbands/warbands-phase1-list-packet-handoff.md` — Phase 1 + 1.5 **debugging record** + code review (**hacks vs stubs vs assumptions**; playtest complete 2026-06-27)
- `docs/midnight-assessment/warbands/warbands-retail-sniff-2026-06-27.md` — retail sniff `12.0.7.68275` (Capture A: Phase 4 bank; Capture B: Phase 1 transfer wire format)
- `docs/midnight-assessment/warbands/warbands-agent-intro.md` — **paste/read first** for agents starting in-game warbands work
- `docs/midnight-assessment/complete/fel-rush-handoff.md` — Fel Rush DH movement, **complete** 2026-06-29 (parent of a 5-doc cluster now archived under `complete/`; air spot-check deferred by owner, not NYI)
- `docs/midnight-assessment/combat-stats-retail-parity-handoff.md` — **Global stat/combat parity** (all classes + creatures) — parent handoff
- `docs/midnight-assessment/combat-stats-retail-parity-phase0-pass-a-handoff.md` — **Phase 0 Pass A** (**complete** — evidence only)
- `docs/midnight-assessment/combat-stats-retail-parity-phase0-pass-b-handoff.md` — **Phase 0 Pass B** (**complete** 2026-06-30 — **FR-B–F**; L80 squish RCA)
- `docs/midnight-assessment/combat-stats-retail-parity-phase-p1-handoff.md` — **Phase P1** base stamina/HP (**CLOSED** 2026-07-01 — Option A + 12.0 squish; owner-playtest-verified; known gap: L81-90 taper, not NYI)
- `docs/midnight-assessment/combat-stats-retail-parity-phase-p2-handoff.md` — **Phase P2** STR/AGI/INT + AttackPower (**CLOSED** 2026-07-01 — 6/13 classes sniff-derived, owner-playtest-verified; known gaps: 7-class fallback weights, L81-90 taper)
- `docs/midnight-assessment/combat-stats-retail-parity-roadmap.md` — combat-stats phases (P/C/S tracks)
- `docs/midnight-assessment/combat-stats-retail-parity-phase-p3-handoff.md` — **Phase P3** base armor (**kicked off, blocked on evidence** 2026-07-01 — `agi*2` confirmed wrong mechanism; needs a 2nd retail armor data point before a replacement formula can be picked)
- `docs/midnight-assessment/combat-stats-retail-parity-contract.md` — combat-stats locked work rules
- `docs/midnight-assessment/dracthyr/dracthyr-forbidden-reach-handoff.md` — side project: Dracthyr intro overview; **§8 evidence/gap labels**
- `docs/midnight-assessment/dracthyr/dracthyr-phase-1b-handoff.md` — Phase 1b lower War Creche (mostly done; playtest bugs B1–B3)
- `docs/midnight-assessment/dracthyr/dracthyr-intro-quest-popup-decline-handoff.md` — **R2 done:** Accept/Decline **64864** (`OfferedScriptQuestID` + `ACCEPT_QUEST` grant)
- `docs/midnight-assessment/dracthyr/dracthyr-b4-azurathel-speak-handoff.md` — B4 Azurathel **181056** speak/turn-in blocker
- `docs/midnight-assessment/dracthyr/dracthyr-intro-opening-retail-parity-handoff.md` — **retail parity** R1–R3 (R2 accept/decline done; R1/R3 open)
- `docs/midnight-assessment/dracthyr/dracthyr-intro-quest-accept-history.md` — R2 chronology (superseded experiments; do not implement from)
- `docs/midnight-assessment/dracthyr/dracthyr-intro-quest-accept-proper-fix-handoff.md` — Hack 1 Track B (done; final architecture in popup-decline handoff)
- `docs/midnight-assessment/dracthyr/dracthyr-intro-quest-accept-fallback-removal-handoff.md` — **done:** Hack 2 (2s fallback) removed
- `docs/midnight-assessment/ascension-inspired-aspirations.md` — **aspirational only** — Ascension-inspired dream features (not a work track)

Prior-attempt gate/handoff docs are archived under
`docs/midnight-assessment/archive/pre-restart/` — not current state.
