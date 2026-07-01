#!/usr/bin/env python3
"""Pass A: creature ExpectedStat HP computation."""
import csv
import math
from functools import reduce
from pathlib import Path

ROOT = Path(__file__).parent

expected = {}
with open(ROOT / "ExpectedStat.csv", newline="", encoding="utf-8-sig") as f:
    for row in csv.DictReader(f):
        if int(row["ExpansionID"]) == -2:
            expected[int(row["Lvl"])] = {k: float(row[k]) for k in row if k not in ("ID", "ExpansionID", "ContentSetID", "Lvl")}

mods = {}
with open(ROOT / "ExpectedStatMod.csv", newline="", encoding="utf-8-sig") as f:
    for row in csv.DictReader(f):
        mods[int(row["ID"])] = {k: float(row[k]) for k in row if k != "ID"}

content_by_tuning = {}
with open(ROOT / "ContentTuningXExpected.csv", newline="", encoding="utf-8-sig") as f:
    for row in csv.DictReader(f):
        tid = int(row["ContentTuningID"])
        content_by_tuning.setdefault(tid, []).append(int(row["ExpectedStatModID"]))

def eval_stat(stat_key, level, expansion, content_tuning_id, class_mod_id=None):
    row = expected.get(level)
    if row is None:
        return 1.0
    base = row[stat_key]
    mod = 1.0
    for mid in content_by_tuning.get(content_tuning_id, []):
        field = stat_key.replace("CreatureAutoAttackDps", "CreatureAutoAttackDPSMod")
        if stat_key == "CreatureHealth":
            field = "CreatureHealthMod"
        elif stat_key == "CreatureArmor":
            field = "CreatureArmorMod"
        elif stat_key == "CreatureAutoAttackDps":
            field = "CreatureAutoAttackDPSMod"
        mod *= mods[mid][field]
    if class_mod_id is not None:
        field = "CreatureHealthMod" if stat_key == "CreatureHealth" else (
            "CreatureAutoAttackDPSMod" if stat_key == "CreatureAutoAttackDps" else "CreatureArmorMod")
        mod *= mods[class_mod_id][field]
    return base * mod

CREATURES = [
    ("99656", "Red Broodling", 1, 81, 11, 0.2, 16, "FR-A L268130"),
    ("99650", "Thornclaw Broodling", 1, 81, 11, 0.2, 16, "FR-A L274257"),
    ("98290", "Cyana Nightglaive", 8, 699, 6, 10, 37180, "FR-A L280180 (native L45)"),
    ("99918", "Sevis Brightflame", 8, 699, 6, 9, 33462, "FR-A L281490 (native L45)"),
    ("99218", "Legion Devastator Missile", 10, 773, 6, 1, 6760, "FR-A L283689 (native L45)"),
    ("99351", "Legion Devastator Missile", 10, 773, 6, 1, 6760, "FR-A L285873 (native L45)"),
    ("94492", "Colossal Infernal", 10, 773, 6, 12, 81120, "FR-A L266923 (native L45)"),
]

print("entry,level,contentTuning,expansion,healthMod,emuHP,retailHP,label")
for entry, name, lvl, ctid, exp, hmod, retail, cite in CREATURES:
    ch = eval_stat("CreatureHealth", lvl, -2, ctid)
    emu = math.ceil(ch * hmod)
    # also try with HealthScalingExpansion from DB (-2 fallback)
    ch2 = eval_stat("CreatureHealth", lvl, exp, ctid)
    emu2 = math.ceil(ch2 * hmod)
    print(f"{entry} L{lvl} ct={ctid} exp=-2: base={ch:.2f} emu={emu} | exp={exp}: base={ch2:.2f} emu={emu2} | retail={retail} {cite}")

print("\nScaled mobs at L8 (player target level):")
for entry, name, _, ctid, exp, hmod, _, cite in CREATURES[2:5]:
    ch = eval_stat("CreatureHealth", 8, -2, ctid)
    emu = math.ceil(ch * hmod)
    print(f"{entry} L8 effective: base={ch:.2f} emuHP={emu} ({name})")

print("\nContentTuning mod chains:")
for tid in [0, 81, 699, 773]:
    ids = content_by_tuning.get(tid, [])
    prod = reduce(lambda a, b: a * mods[b]["PlayerHealthMod"], ids, 1.0) if ids else 1.0
    cprod = reduce(lambda a, b: a * mods[b]["CreatureHealthMod"], ids, 1.0) if ids else 1.0
    print(f"  tuning {tid}: mod IDs {ids} -> PlayerHealthMod product={prod:.4f}, CreatureHealthMod product={cprod:.4f}")
