# Local DB2 CSV cache (`temp/db2/`)

**Git:** This README is tracked; **CSV exports and manifests are gitignored** (local only).

**Workflow:** [`docs/midnight-assessment/db2-export-workflow.md`](../docs/midnight-assessment/db2-export-workflow.md)

---

## Layout

```text
temp/db2/
  README.md                 ← you are here (committed)
  12.0.7.68275/             ← one folder per client build string
    manifest.txt            ← build, date, spell ids, source, handoff link
    SpellEffect-fel-rush.csv
    SpellTargetRestrictions-fel-rush.csv
    …
```

Use the build from worldserver `DataDir` (e.g. `WOWSTATIC_12_0_7_68275` → `12.0.7.68275`).

---

## manifest.txt template

```text
build=12.0.7.68275
exported=YYYY-MM-DD
source=wago.tools
spell_ids=344865,195072,197922,197923,192611
tables=SpellEffect,SpellTargetRestrictions
handoff=docs/midnight-assessment/fel-rush-handoff.md
```

---

## For agents

1. If a handoff cites `temp/db2/<build>/`, read `manifest.txt` first.
2. Grep the CSV files — do not re-parse binary `.db2` unless manifest is missing.
3. If the folder is empty, run `.\tools\setup-db2-tools.ps1` then `EvryDb2Export export-spells …`, or use wago per [`db2-export-workflow.md`](../docs/midnight-assessment/db2-export-workflow.md).
4. **`detect-build` wins** over handoff client build when they differ (DataDir vs retail sniff).

---

## Owner refresh

After a client/DBC update:

1. Confirm new build string from `SpellEffect.db2` header.
2. Create `temp/db2/<new-build>/` and re-export filtered CSVs.
3. Update feature handoff evidence tables; old build folders can be deleted locally.
