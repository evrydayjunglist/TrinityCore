#!/usr/bin/env python3
"""Pass A scratch: EvaluateExpectedStat PlayerHealth prediction (Option A)."""
import csv
from pathlib import Path

ROOT = Path(__file__).parent
LEVELS = [1, 8, 20, 40, 60, 70, 80]
# CLASS_WARRIOR=1, CLASS_MAGE=8, CLASS_DEMON_HUNTER=12
CLASSES = {1: ("Warrior", 4), 8: ("Mage", 1), 12: ("DH", None)}

# ExpectedStat by level (ExpansionID=-2)
expected = {}
with open(ROOT / "ExpectedStat.csv", newline="", encoding="utf-8-sig") as f:
    for row in csv.DictReader(f):
        if int(row["ExpansionID"]) == -2:
            expected[int(row["Lvl"])] = row

# ExpectedStatMod by ID
mods = {}
with open(ROOT / "ExpectedStatMod.csv", newline="", encoding="utf-8-sig") as f:
    for row in csv.DictReader(f):
        mods[int(row["ID"])] = float(row["PlayerHealthMod"])

# ContentTuningXExpected for contentTuningId=0
content_mods = []
with open(ROOT / "ContentTuningXExpected.csv", newline="", encoding="utf-8-sig") as f:
    for row in csv.DictReader(f):
        if int(row["ContentTuningID"]) == 0:
            content_mods.append(int(row["ExpectedStatModID"]))

# HpPerSta
hp_per_sta = {}
with open(ROOT / "HpPerSta.txt", encoding="utf-8") as f:
    next(f)
    for line in f:
        parts = line.strip().split("\t")
        if len(parts) >= 2:
            hp_per_sta[int(parts[0])] = float(parts[1])

def eval_player_health(level, class_id, content_tuning_id=0):
    base = float(expected[level]["PlayerHealth"])
    mod = 1.0
    if content_tuning_id == 0:
        for mid in content_mods:
            mod *= mods[mid]
    class_mod_id = CLASSES[class_id][1]
    if class_mod_id is not None:
        mod *= mods[class_mod_id]
    return base * mod

print("ContentTuningID=0 ExpectedStatMod IDs:", content_mods)
print("Content mod product:", __import__("functools").reduce(lambda a, b: a * mods[b], content_mods, 1.0))
print()
print("Level,Class,PlayerHealth,Stamina,MaxHP,HpPerSta")
for cls_id, (name, _) in CLASSES.items():
    for lvl in LEVELS:
        ph = eval_player_health(lvl, cls_id)
        hps = hp_per_sta[lvl]
        sta = round(ph / hps)
        mhp = sta * hps
        print(f"{lvl},{name}({cls_id}),{ph:.2f},{sta},{mhp:.0f},{hps}")
