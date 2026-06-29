# Agent Instructions — TrinityCore (Midnight)

**Full onboarding for new agents:** read [`docs/new-agent-intro.md`](docs/new-agent-intro.md) first.

**Fork steering (canonical):** [`docs/midnight-assessment/project-focus.md`](docs/midnight-assessment/project-focus.md)

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
| **Primary** | AC-like `modules/` + native gated `mod-playerbots` (Phase 2 next) |
| **Alternate** | Retail-parity content (Dracthyr intro today) |

Default path for all tracks: human Battle.net login on `MODULES=none`.

**Fix quality (fork mottos):** **Non-hacky by default** and **We don't break things**
— retail/sniff/DB2 evidence, minimal portable diffs, baseline protected. **NYI**
only for genuine future feature work —
[`project-focus.md` § NYI vocabulary](docs/midnight-assessment/project-focus.md#nyi-vocabulary--the-only-defer-reason)
(**finish now unless NYI**).
Full bar: [`project-focus.md` § Fork mottos](docs/midnight-assessment/project-focus.md#fork-mottos).
Scoped tasks: [`agent-task-handoff-template.md`](docs/midnight-assessment/agent-task-handoff-template.md).

**SQL (fork):** new tables/data → **`sql/updates/{world,auth,characters,hotfixes}/master/`**
only — **do not patch `sql/base/dev/`** unless the owner explicitly requests a full
base refresh. Existing dev DBs apply changes via **`worldserver -u`**. See
[`sql-update-conventions.md`](docs/midnight-assessment/sql-update-conventions.md).

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

**Primary (special interest):**

- **AzerothCore-like:** modular, config-driven, small gates, baseline protected
- **mod-playerbots-like:** real player bots/sessions — **no** WotLK wholesale import

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
- `docs/midnight-assessment/fork-journal.md` — infra, upstream merges, repo hygiene chronology
- `docs/midnight-assessment/successful-local-baseline.md` — build/run/login (confirmed 2026-06-25; re-verify after upstream merges)
- `docs/midnight-assessment/build-and-run-readiness.md` — build/run gates + module pitfalls
- `docs/midnight-assessment/upstream-core-maintenance.md` — server log triage; upstream drift + ROI fix backlog; **retail-ready by default** (owner approval for non-retail stubs); **agent validation warnings** (grep ≠ gameplay, `GetAura`, port **8085**)
- `docs/midnight-assessment/upstream-core-maintenance-rank1-handoff.md` — Rank 1 startup quick wins (**complete**)
- `docs/midnight-assessment/upstream-core-maintenance-rank2-handoff.md` — Rank 2 guild challenge re-enqueue (**complete** 2026-06-28)
- `docs/midnight-assessment/upstream-core-maintenance-rank2b-handoff.md` — Rank 2b garrison / splash / empower re-enqueue (**complete** 2026-06-28)
- `docs/midnight-assessment/upstream-core-maintenance-rank2c-handoff.md` — Rank 2c catalog / quick-join re-enqueue (**complete** 2026-06-28; Capture J validated)
- `docs/midnight-assessment/upstream-core-maintenance-rank2d-handoff.md` — Rank 2d account-UI login re-enqueue batch (**complete** 2026-06-28; Capture L validated)
- `docs/midnight-assessment/upstream-core-maintenance-r1a-r2c2-retail-parity-handoff.md` — **R1-A** `PvpSeason.db2` + **R2e/R2d C2** SMSG timing (**complete** 2026-06-28; Capture O/P validated)
- `docs/midnight-assessment/upstream-core-maintenance-r1a-tierc-handoff.md` — **R1-A Tier C** `SMSG_SEASON_INFO` + DB2 season (**complete** 2026-06-28; owner login pass)
- `docs/midnight-assessment/upstream-core-maintenance-r1a-tierc-f1-displayseason-gaps-handoff.md` — **R1-A Tier C F1** DisplaySeason store + gap fields **117** / **1069** (**complete** 2026-06-28)
- `docs/midnight-assessment/upstream-core-maintenance-rank2e-handoff.md` — Rank 2e login re-enqueue tail (**complete** 2026-06-28; Capture N validated)
- `docs/midnight-assessment/upstream-core-maintenance-rank3-rank4-handoff.md` — Rank 3–4 batch spell scripts (**complete** 2026-06-28)
- `docs/midnight-assessment/upstream-core-maintenance-rank5-handoff.md` — Rank 5 cascading missing spell ids (**complete** 2026-06-28; R5-A playtest + GetAura follow-up)
- `docs/midnight-assessment/upstream-core-maintenance-rank7-handoff.md` — Rank 7 warband groups CMSG wire parse (**complete** 2026-06-28; C2/C3 owner-validated)
- `docs/midnight-assessment/upstream-core-maintenance-rank8-handoff.md` — Rank 8 unhandled opcode stubs (**complete** 2026-06-28; **19/19** wired; owner-validated **25 → 0**)
- `docs/midnight-assessment/upstream-core-maintenance-rank8-f2-f5-sniff-upstream-handoff.md` — **R8-F2** stub SMSG sniff (**complete** 2026-06-28)
- `docs/midnight-assessment/housing/housing-r8-f4-get-player-houses-info-handoff.md` — **R8-F4** housing login stub (**complete** 2026-06-28; owner R8-F4 pass)
- `docs/midnight-assessment/upstream-core-maintenance-rank9-handoff.md` — Rank 9 DBC mismatch batch (**complete** 2026-06-28; **138 → 0**; **no follow-ups**; § Phase 3B/3C planning; Warnings 7–9 in parent spec). **Next optional upstream ROI:** rank **10** or **11** — parent spec ROI table.
- `docs/midnight-assessment/upstream-core-maintenance-rank2-retail-sniff-2026-06-27.md` — Rank 2 retail packet evidence (Captures C/D live retail; not evry realm)
- `docs/midnight-assessment/module-support-prereq.md` — module guardrails (target design)
- `docs/midnight-assessment/playerbots/playerbots-integration-plan.md` — Playerbots constraints and phases
- `scripts/build-trinitycore-master.ps1` — canonical local build script (auto-detects evry paths)
- `scripts/README.md` — fork helper script inventory (build, log triage, validators, sniff)
- `scripts/validate-rank5-deaths-embrace.ps1` — Rank 5 R5-A guard (code + log grep + SpellEffect layout)
- `doc/` — upstream TrinityCore how-tos (`HowToScript.txt`, `UnixInstall.txt`, …)
- `docs/midnight-assessment/reference-trees-and-standards.md` — reference tree paths + AC/module standards
- `docs/midnight-assessment/sql-update-conventions.md` — SQL update naming + upstream sync
- `docs/midnight-assessment/retail-data-first-checklist.md` — **before hardcoding client ids:** grep DB2, map packet fields, self-review
- `docs/midnight-assessment/retail-packet-sniff-workflow.md` — **live retail** Ymir + WowPacketParser agent/owner loop
- `docs/midnight-assessment/agent-task-handoff-template.md` — **paste/read first** for scoped agent tasks (rules, sworn acknowledgment, task brief, closeout)
- `docs/midnight-assessment/warbands/char-select-campsites-handoff.md` — char-select campsites (**complete** user 2026-06-27; not in-game housing)
- `docs/midnight-assessment/warbands/warbands-overall-handoff.md` — **warbands overall** (Phases 1 + 1.5 + **2 complete** user 2026-06-27; Phase 3–4 next)
- `docs/midnight-assessment/warbands/warbands-phase2-account-wide-currency-handoff.md` — Phase 2 account-wide currency **complete** (implementation spec + R1–R3 follow-ups)
- `docs/midnight-assessment/warbands/warbands-phase2plus-account-wide-earning-handoff.md` — Phase 2+ account-wide earning eligibility (deferred)
- `docs/midnight-assessment/warbands/warbands-renown-stub-handoff.md` — **Renown stub** (Adventure Guide UI + quest gates; deferred — backend currency done)
- `docs/midnight-assessment/warbands/warbands-phase3-warband-reputation-handoff.md` — Phase 3 warband reputation (**next** implementation spec)
- `docs/midnight-assessment/warbands/warbands-phase1-list-packet-handoff.md` — Phase 1 + 1.5 **debugging record** + code review (**hacks vs stubs vs assumptions**; playtest complete 2026-06-27)
- `docs/midnight-assessment/warbands/warbands-retail-sniff-2026-06-27.md` — retail sniff `12.0.7.68275` (Capture A: Phase 4 bank; Capture B: Phase 1 transfer wire format)
- `docs/midnight-assessment/warbands/warbands-agent-intro.md` — **paste/read first** for agents starting in-game warbands work
- `docs/midnight-assessment/dracthyr/dracthyr-forbidden-reach-handoff.md` — side project: Dracthyr intro overview; **§8 evidence/gap labels**
- `docs/midnight-assessment/dracthyr/dracthyr-phase-1b-handoff.md` — Phase 1b lower War Creche (mostly done; playtest bugs B1–B3)
- `docs/midnight-assessment/dracthyr/dracthyr-intro-quest-popup-decline-handoff.md` — **R2 done:** Accept/Decline **64864** (`OfferedScriptQuestID` + `ACCEPT_QUEST` grant)
- `docs/midnight-assessment/dracthyr/dracthyr-b4-azurathel-speak-handoff.md` — B4 Azurathel **181056** speak/turn-in blocker
- `docs/midnight-assessment/dracthyr/dracthyr-intro-opening-retail-parity-handoff.md` — **retail parity** R1–R3 (R2 accept/decline done; R1/R3 open)
- `docs/midnight-assessment/dracthyr/dracthyr-intro-quest-accept-history.md` — R2 chronology (superseded experiments; do not implement from)
- `docs/midnight-assessment/dracthyr/dracthyr-intro-quest-accept-proper-fix-handoff.md` — Hack 1 Track B (done; final architecture in popup-decline handoff)
- `docs/midnight-assessment/dracthyr/dracthyr-intro-quest-accept-fallback-removal-handoff.md` — **done:** Hack 2 (2s fallback) removed
- `modules/README.md` — product module layout

Prior-attempt gate/handoff docs are archived under
`docs/midnight-assessment/archive/pre-restart/` — not current state.
