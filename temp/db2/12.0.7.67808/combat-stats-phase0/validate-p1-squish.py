#!/usr/bin/env python3
"""P1 validation: mirror the exact C++ logic added to Player::InitStatsForLevel
(contentTuningId=0, classMod, squish gated on level>=80) and compare against the
locked retail acceptance anchors in combat-stats-retail-parity-phase-p1-handoff.md §3.
"""
import csv
from pathlib import Path

ROOT = Path(__file__).parent
# CLASS_WARRIOR=1, CLASS_MAGE=8, CLASS_DEMON_HUNTER=12 (no ExpectedStatMod branch for DH)
CLASSES = {1: ("Warrior", 4), 8: ("Mage", 1), 12: ("DH", None)}

expected = {}
with open(ROOT / "ExpectedStat.csv", newline="", encoding="utf-8-sig") as f:
    for row in csv.DictReader(f):
        if int(row["ExpansionID"]) == -2:
            expected[int(row["Lvl"])] = row

mods = {}
with open(ROOT / "ExpectedStatMod.csv", newline="", encoding="utf-8-sig") as f:
    for row in csv.DictReader(f):
        mods[int(row["ID"])] = float(row["PlayerHealthMod"])

content_mods = []
with open(ROOT / "ContentTuningXExpected.csv", newline="", encoding="utf-8-sig") as f:
    for row in csv.DictReader(f):
        if int(row["ContentTuningID"]) == 0:
            content_mods.append(int(row["ExpectedStatModID"]))

hp_per_sta = {}
with open(ROOT / "HpPerSta.txt", encoding="utf-8") as f:
    next(f)
    for line in f:
        parts = line.strip().split("\t")
        if len(parts) >= 2:
            hp_per_sta[int(parts[0])] = float(parts[1])

SQUISH = 0.9  # Player.cpp InitStatsForLevel literal -- Pass B FR-D/E/F sniff, see §3.5

def expected_player_health(level, class_id):
    base = float(expected[level]["PlayerHealth"])
    mod = 1.0
    for mid in content_mods:  # contentTuningId == 0
        mod *= mods[mid]
    class_mod_id = CLASSES[class_id][1]
    if class_mod_id is not None:
        mod *= mods[class_mod_id]
    return base * mod

def p1_stamina_and_hp(level, class_id):
    ph = expected_player_health(level, class_id)
    if level >= 80:  # Trinity::GetExpansionForLevel(level) == CURRENT_EXPANSION
        ph *= SQUISH
    hps = hp_per_sta[level]
    sta = max(round(ph / hps), 1)
    return sta, sta * hps

ANCHORS = [
    # (class_id, level, retail_MaxHP, retail_base_sta_or_None, note)
    (1, 1, 292, None, "FR-B WAR L1"),
    (8, 1, 292, None, "FR-C MAG L1"),
    (12, 8, None, 497, "FR-A DH L8 base (total 518 = base + StatPosBuff +21, out of P1 scope)"),
    (1, 80, 46540, 2327, "FR-D WAR L80 naked"),
    (8, 80, 46520, 2326, "FR-E MAG L80 naked"),
    (12, 80, 46500, 2325, "FR-F DH L80 naked"),
]

print(f"{'Class':<8}{'Lvl':>4}{'STA':>8}{'MaxHP':>10}{'RetailHP':>12}{'RetailSTA':>11}{'Match':>8}")
for cls_id, lvl, retail_hp, retail_sta, note in ANCHORS:
    name = CLASSES[cls_id][0]
    sta, hp = p1_stamina_and_hp(lvl, cls_id)
    hp_ok = (retail_hp is None) or abs(hp - retail_hp) <= 1
    sta_ok = (retail_sta is None) or abs(sta - retail_sta) <= 1
    match = "OK" if hp_ok and sta_ok else "DRIFT"
    print(f"{name:<8}{lvl:>4}{sta:>8}{hp:>10.0f}{(retail_hp if retail_hp is not None else '-'):>12}{(retail_sta if retail_sta is not None else '-'):>11}{match:>8}  # {note}")
