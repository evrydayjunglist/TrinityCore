# Fork helper scripts (`scripts/`)

PowerShell and Python helpers for **this TrinityCore-evry checkout** — build,
log triage, retail sniff, and maintenance validation.

**Not** `src/server/scripts/` (built-in C++ game scripts — `SCRIPTS` CMake option).

Full paths and client builds: [`docs/midnight-assessment/successful-local-baseline.md`](../docs/midnight-assessment/successful-local-baseline.md).

---

## Build

| Script | Purpose |
|--------|---------|
| [`build-trinitycore-master.ps1`](build-trinitycore-master.ps1) | Canonical **worldserver** / **bnetserver** build. Auto-detects evry source + `Builds/TrinityCore-evry`. Default `-Modules none`. |
| [`build-trinitycore-tools.ps1`](build-trinitycore-tools.ps1) | Extractor / map / vmap / mmaps tools (`TOOLS=1`). Same build dir as master script. |
| [`trinitycore-build-common.ps1`](trinitycore-build-common.ps1) | Shared CMake reconfigure helpers — sourced by build scripts and validators. |

**Doc:** [`build-and-run-readiness.md`](../docs/midnight-assessment/build-and-run-readiness.md),
[`successful-local-baseline.md`](../docs/midnight-assessment/successful-local-baseline.md).

---

## Upstream log maintenance

| Script | Purpose |
|--------|---------|
| [`parse-server-log-inventory.py`](parse-server-log-inventory.py) | Parse `Server.log` → typed inventory JSON + regenerate [`_server-log-catalog.generated.md`](../docs/midnight-assessment/_server-log-catalog.generated.md). |

```powershell
python scripts/parse-server-log-inventory.py D:/WOWEmulation/Emulators/Builds/TrinityCore-evry/bin/RelWithDebInfo/logs/Server.log
```

**Doc:** [`upstream-core-maintenance.md`](../docs/midnight-assessment/upstream-core-maintenance.md).

| Script | Purpose |
|--------|---------|
| [`validate-rank5-deaths-embrace.ps1`](validate-rank5-deaths-embrace.ps1) | **Rank 5 R5-A guard** — `spell_warlock.cpp` must not use masked `GetAura` on Death's Embrace; startup log grep for four Rank 5 ids; wago `SpellEffect` layout for **`234876`**. |

```powershell
.\scripts\validate-rank5-deaths-embrace.ps1
```

**Doc:** [`upstream-core-maintenance-rank5-handoff.md`](../docs/midnight-assessment/upstream-core-maintenance-rank5-handoff.md),
[`upstream-core-maintenance.md` § Agent validation warnings](../docs/midnight-assessment/upstream-core-maintenance.md#agent-validation-warnings-maintenance-track).

| Script | Purpose |
|--------|---------|
| [`query-spell-effect-db2.py`](query-spell-effect-db2.py) | **Optional offline** dump of `SpellEffect.db2` rows for one or more spell ids (EffectIndex, Effect, Aura, ImplicitTarget). Reads loaded build under `Builds/TrinityCore-evry/.../data/dbc/enUS/`. |

```powershell
python scripts/query-spell-effect-db2.py 381753 120517 47788
```

**When to use:** batch DBC mismatch research when wago is slow or offline. **Prefer wago + curl** for closeout evidence ([`retail-data-first-checklist.md` § 7](../docs/midnight-assessment/retail-data-first-checklist.md#7-spell--db2-evidence-local-checkout)). Always **spot-check** local output against wago before committing hook rebinds ([`rank9 handoff` § Phase 3B planning](../docs/midnight-assessment/upstream-core-maintenance-rank9-handoff.md#phase-3b-planning--what-worked-for-future-batches)).

---

## Live retail packet capture

| Script | Purpose |
|--------|---------|
| [`retail-sniff-capture.ps1`](retail-sniff-capture.ps1) | Ymir + WowPacketParser workflow — Prepare / StartCapture / Parse / Status. |

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\retail-sniff-capture.ps1 -Action Prepare
```

**Doc:** [`retail-packet-sniff-workflow.md`](../docs/midnight-assessment/retail-packet-sniff-workflow.md).

---

## Scratch artifacts (do not commit)

Agent research dumps under `scripts/` (e.g. `_p3b_spelleffect*.csv`) are **local scratch** — not fork tools. Prefer `temp/` (gitignored) for CSV exports.
