#!/usr/bin/env python3
"""Extract unique Server.log message types for upstream-core-maintenance inventory."""

from __future__ import annotations

import json
import re
import sys
from collections import defaultdict
from pathlib import Path


def classify(line: str) -> dict | None:
    line = line.rstrip("\n")
    if not line.strip():
        return None

    m = re.match(r"SpellScriptBase::ValidateSpellInfo: Spell (\d+) does not exist\.", line)
    if m:
        return {
            "section_id": "spell_validate_missing_spell_info",
            "instance_spell": int(m.group(1)),
        }

    m = re.match(
        r"SpellScriptBase::ValidateSpellEffect: Spell (\d+) does not have (EFFECT_\d+)\.",
        line,
    )
    if m:
        return {
            "section_id": "spell_validate_missing_effect_slot",
            "instance_spell": int(m.group(1)),
            "instance_effect": m.group(2),
        }

    m = re.match(r"SpellScriptBase::ValidateSpellEffect: Spell (\d+) does not exist\.", line)
    if m:
        return {
            "section_id": "spell_validate_effect_spell_missing",
            "instance_spell": int(m.group(1)),
        }

    m = re.match(
        r"Spell `(\d+)` did not pass Validate\(\) function of script `([^`]+)` - script will be not added to the spell",
        line,
    )
    if m:
        return {
            "section_id": "spell_validate_failed",
            "instance_spell": int(m.group(1)),
            "instance_script": m.group(2),
        }

    m = re.match(
        r"Spell `(\d+)` of script `([^`]+)` does not have apply aura effect - handler bound to hook `([^`]+)` of AuraScript won't be executed",
        line,
    )
    if m:
        return {
            "section_id": "spell_no_apply_aura_effect",
            "instance_spell": int(m.group(1)),
            "instance_script": m.group(2),
            "hook": m.group(3),
        }

    m = re.match(
        r"Spell `(\d+)` script `([^`]+)` does not have a damage effect - handler bound to hook `([^`]+)` of SpellScript won't be executed",
        line,
    )
    if m:
        return {
            "section_id": "spell_no_damage_effect",
            "instance_spell": int(m.group(1)),
            "instance_script": m.group(2),
            "hook": m.group(3),
        }

    m = re.search(
        r"did not match dbc effect data - handler bound to hook `([^`]+)` of (AuraScript|SpellScript) won't be executed",
        line,
    )
    if m and line.startswith("Spell `"):
        spell_m = re.match(r"Spell `(\d+)`", line)
        script_m = re.search(r"of script `([^`]+)`", line)
        return {
            "section_id": f"spell_dbc_mismatch_{m.group(1)}_{m.group(2)}",
            "hook": m.group(1),
            "script_type": m.group(2),
            "instance_spell": int(spell_m.group(1)) if spell_m else None,
            "instance_script": script_m.group(1) if script_m else None,
        }

    m = re.search(r"DB table `([^`]+)` is empty", line)
    if m and ">> Loaded 0" in line:
        return {"section_id": "db_table_empty", "instance_table": m.group(1)}

    if line.startswith("Invalid BattlegroundMapScript for "):
        map_m = re.search(r"for (\d+)", line)
        return {
            "section_id": "invalid_battleground_map_script",
            "instance_map": int(map_m.group(1)) if map_m else None,
        }

    if "could not be found in BattlemasterList.dbc" in line:
        bg_m = re.search(r"Battleground ID (\d+)", line)
        return {
            "section_id": "battleground_missing_from_dbc",
            "instance_bg_id": int(bg_m.group(1)) if bg_m else None,
        }

    if line.startswith("ArenaSeason ("):
        return {"section_id": "arena_season_invalid"}

    if line.startswith("Tried to override client handler"):
        return {"section_id": "opcode_handler_duplicate_override"}

    m = re.search(r"Received not handled opcode \[([^\]]+)\]", line)
    if m:
        return {"section_id": "unhandled_opcode", "instance_opcode": m.group(1)}

    m = re.search(r"Re-enqueueing packet with opcode \[([^\]]+)\].*status (\w+)", line)
    if m:
        return {
            "section_id": "packet_reenqueue_before_in_world",
            "instance_opcode": m.group(1),
            "status": m.group(2),
        }

    if "ByteBufferException" in line:
        opcode_m = re.search(r"opcode: \[([^\]]+)\]", line)
        return {
            "section_id": "bytebuffer_parse_exception",
            "instance_opcode": opcode_m.group(1) if opcode_m else None,
        }

    if "not implemented method" in line:
        method_m = re.search(r"not implemented method (\S+)", line)
        return {
            "section_id": "client_service_not_implemented",
            "instance_method": method_m.group(1) if method_m else None,
        }

    if line.startswith("WORLD: Received CMSG_QUEST_QUERY"):
        return {"section_id": "world_debug_quest_query"}

    if line.startswith("WORLD: Sent SMSG_QUEST_QUERY_RESPONSE"):
        return {"section_id": "world_debug_quest_query_response"}

    if line.startswith("WORLD: Sent SMSG_QUESTGIVER_STATUS"):
        return {"section_id": "world_debug_questgiver_status"}

    if line.startswith("WORLD: Sent SMSG_GAMEOBJECT_QUERY_RESPONSE"):
        return {"section_id": "world_debug_gameobject_query_response"}

    if line.startswith("WORLD: Sent SMSG_AI_REACTION"):
        return {"section_id": "world_debug_ai_reaction"}

    if line.startswith("CMSG_CALENDAR_GET_NUM_PENDING"):
        return {"section_id": "calendar_pending_debug"}

    if line.startswith("SESSION: Sent SMSG_LOGOUT_COMPLETE"):
        return {"section_id": "session_logout_complete_debug"}

    if line.startswith("Player::SendInitWorldStates"):
        return {"section_id": "player_init_world_states_debug"}

    if line.startswith("Loading char guid"):
        return {"section_id": "session_char_load_debug"}

    if line.startswith("Character ") and " logging in" in line:
        return {"section_id": "session_character_login_debug"}

    if "HandleAuthSession" in line and "authenticated successfully" in line:
        return {"section_id": "session_auth_success_debug"}

    if line.startswith("Allowed Level:"):
        return {"section_id": "session_allowed_level_debug"}

    if "ReadDataHandler" in line and "CMSG_LOG_DISCONNECT" in line:
        return {"section_id": "session_client_disconnect_debug"}

    if line.startswith("WORLD:"):
        if "MMap data directory" in line or "Received CMSG_UPDATE_ACCOUNT_DATA" in line:
            return None
        return {"section_id": "world_debug_other", "raw_prefix": line[:80]}

    return None


SECTION_META = {
    "spell_validate_missing_spell_info": {
        "title": "Spell validation — referenced spell missing (ValidateSpellInfo)",
        "pattern": "SpellScriptBase::ValidateSpellInfo: Spell {id} does not exist.",
        "severity": "P3",
        "fix": "Update or remove script reference; spell removed/renumbered in retail DBC.",
    },
    "spell_validate_missing_effect_slot": {
        "title": "Spell validation — effect slot missing (ValidateSpellEffect)",
        "pattern": "SpellScriptBase::ValidateSpellEffect: Spell {id} does not have EFFECT_{n}.",
        "severity": "P3",
        "fix": "Rebind script hook to an effect index that exists in current Spell.db2.",
    },
    "spell_validate_effect_spell_missing": {
        "title": "Spell validation — spell missing for effect check",
        "pattern": "SpellScriptBase::ValidateSpellEffect: Spell {id} does not exist.",
        "severity": "P3",
        "fix": "Spell gone from DBC; remove script registration or update spell id.",
    },
    "spell_validate_failed": {
        "title": "Spell validation — Validate() failed (script rejected)",
        "pattern": "Spell `{id}` did not pass Validate() function of script `{name}` - script will be not added to the spell",
        "severity": "P2-P3",
        "fix": "Full script review against current DBC; often chained missing spell/effect refs.",
    },
    "spell_no_apply_aura_effect": {
        "title": "Spell validation — no apply aura effect for hook",
        "pattern": "Spell `{id}` of script `{name}` does not have apply aura effect - handler bound to hook `{hook}` of AuraScript won't be executed",
        "severity": "P3",
        "fix": "Script expects apply-aura effect; spell structure changed.",
    },
    "spell_no_damage_effect": {
        "title": "Spell validation — no damage effect for hook",
        "pattern": "Spell `{id}` script `{name}` does not have a damage effect - handler bound to hook `{hook}` of SpellScript won't be executed",
        "severity": "P3",
        "fix": "Script expects damage effect; spell structure changed.",
    },
    "db_table_empty": {
        "title": "Empty DB table (informational)",
        "pattern": ">> Loaded 0 … DB table `{table}` is empty.",
        "severity": "P4",
        "fix": "None — normal on dev/private realms.",
    },
    "invalid_battleground_map_script": {
        "title": "Invalid BattlegroundMapScript — map id not found",
        "pattern": "Invalid BattlegroundMapScript for {mapId}; no such map ID.",
        "severity": "P2",
        "fix": "Remove stale script registration or add map; check battleground_scripts / C++ scripts.",
    },
    "battleground_missing_from_dbc": {
        "title": "Battleground template missing from BattlemasterList.dbc",
        "pattern": "Battleground ID {id} could not be found in BattlemasterList.dbc. The battleground was not created.",
        "severity": "P2",
        "fix": "Align world DB battleground_template with DBC or remove stale row.",
    },
    "arena_season_invalid": {
        "title": "Arena season id not in DBC",
        "pattern": "ArenaSeason ({season}) must be an existing Arena Season.",
        "severity": "P2",
        "fix": "Fix worldserver.conf / DB arena season vs ArenaSeason.db2.",
    },
    "opcode_handler_duplicate_override": {
        "title": "Duplicate opcode handler registration",
        "pattern": "Tried to override client handler of {opcodeA} with {opcodeB} (opcode {num})",
        "severity": "P2",
        "fix": "Remove duplicate handler registration in Opcodes.cpp / packet handlers.",
    },
    "unhandled_opcode": {
        "title": "Unhandled client opcode",
        "pattern": "Received not handled opcode [{opcode}] from [Player: …]",
        "severity": "P3-P4",
        "fix": "Stub handler if UI breaks; otherwise ignore.",
    },
    "packet_reenqueue_before_in_world": {
        "title": "Packet re-enqueued before player in world",
        "pattern": "Re-enqueueing packet with opcode [{opcode}] with status STATUS_LOGGEDIN. Player is currently not in world yet.",
        "severity": "P4",
        "fix": "Usually benign login timing; implement handler or defer if feature breaks.",
    },
    "bytebuffer_parse_exception": {
        "title": "ByteBuffer parse exception (packet wire mismatch)",
        "pattern": "WorldSession::Update ByteBufferException … parsing a packet (opcode: [{opcode}]) … Skipped packet.",
        "severity": "P0-P1",
        "fix": "Fix packet read/write struct vs retail client; check fork handoffs if warbands/custom.",
    },
    "client_service_not_implemented": {
        "title": "Blizzard client service RPC not implemented",
        "pattern": "Client tried to call not implemented method {service}.{method}(…)",
        "severity": "P4",
        "fix": "Expected on private servers; ignore unless client hangs.",
    },
    "world_debug_quest_query": {
        "title": "WORLD debug — CMSG_QUEST_QUERY",
        "pattern": "WORLD: Received CMSG_QUEST_QUERY quest = {id}",
        "severity": "P4",
        "fix": "Log noise — reduce LogLevel / Logger filters if undesired.",
    },
    "world_debug_quest_query_response": {
        "title": "WORLD debug — SMSG_QUEST_QUERY_RESPONSE",
        "pattern": "WORLD: Sent SMSG_QUEST_QUERY_RESPONSE questid={id}",
        "severity": "P4",
        "fix": "Log noise.",
    },
    "world_debug_questgiver_status": {
        "title": "WORLD debug — SMSG_QUESTGIVER_STATUS",
        "pattern": "WORLD: Sent SMSG_QUESTGIVER_STATUS NPC=…, status=…",
        "severity": "P4",
        "fix": "Log noise.",
    },
    "world_debug_gameobject_query_response": {
        "title": "WORLD debug — SMSG_GAMEOBJECT_QUERY_RESPONSE",
        "pattern": "WORLD: Sent SMSG_GAMEOBJECT_QUERY_RESPONSE",
        "severity": "P4",
        "fix": "Log noise.",
    },
    "world_debug_ai_reaction": {
        "title": "WORLD debug — SMSG_AI_REACTION",
        "pattern": "WORLD: Sent SMSG_AI_REACTION, type …",
        "severity": "P4",
        "fix": "Log noise.",
    },
    "calendar_pending_debug": {
        "title": "Calendar pending count debug",
        "pattern": "CMSG_CALENDAR_GET_NUM_PENDING: [Player-…] Pending: {n}",
        "severity": "P4",
        "fix": "Log noise.",
    },
    "session_logout_complete_debug": {
        "title": "SESSION debug — logout complete",
        "pattern": "SESSION: Sent SMSG_LOGOUT_COMPLETE Message",
        "severity": "P4",
        "fix": "Log noise.",
    },
    "player_init_world_states_debug": {
        "title": "Player init world states debug",
        "pattern": "Player::SendInitWorldStates: Sending SMSG_INIT_WORLD_STATES for Map: …, Zone: …",
        "severity": "P4",
        "fix": "Log noise.",
    },
    "session_char_load_debug": {
        "title": "Character load debug",
        "pattern": "Loading char guid Player-… from account …",
        "severity": "P4",
        "fix": "Normal session flow.",
    },
    "session_character_login_debug": {
        "title": "Character login debug",
        "pattern": "Character Player-… logging in",
        "severity": "P4",
        "fix": "Normal session flow.",
    },
    "session_auth_success_debug": {
        "title": "Auth session success debug",
        "pattern": "WorldSocket::HandleAuthSession: Client '…' authenticated successfully from …",
        "severity": "P4",
        "fix": "Normal session flow.",
    },
    "session_allowed_level_debug": {
        "title": "Allowed level debug",
        "pattern": "Allowed Level: … Player Level …",
        "severity": "P4",
        "fix": "Normal session flow.",
    },
    "session_client_disconnect_debug": {
        "title": "Client disconnect debug",
        "pattern": "WorldSocket::ReadDataHandler: client … sent CMSG_LOG_DISCONNECT reason …",
        "severity": "P4",
        "fix": "Normal session flow.",
    },
    "world_debug_other": {
        "title": "WORLD debug — other",
        "pattern": "WORLD: …",
        "severity": "P4",
        "fix": "Log noise.",
    },
}


def dbc_mismatch_meta(section_id: str) -> dict:
    # spell_dbc_mismatch_{Hook}_{AuraScript|SpellScript}
    rest = section_id.removeprefix("spell_dbc_mismatch_")
    if rest.endswith("_AuraScript"):
        hook = rest[: -len("_AuraScript")]
        script_type = "AuraScript"
    elif rest.endswith("_SpellScript"):
        hook = rest[: -len("_SpellScript")]
        script_type = "SpellScript"
    else:
        hook = rest
        script_type = "?"
    return {
        "title": f"Spell DBC mismatch — {hook} ({script_type})",
        "pattern": f"Spell `{{id}}` Effect `…` of script `{{name}}` did not match dbc effect data - handler bound to hook `{hook}` of {script_type} won't be executed",
        "severity": "P3",
        "fix": "Update effect index/aura/target in script to match current Spell.db2 layout.",
        "hook": hook,
        "script_type": script_type,
    }


def render_markdown_catalog(result: dict) -> str:
    lines: list[str] = []
    lines.append("## Complete log accounting\n")
    lines.append(f"Source log: `{result['log_path']}`\n")
    lines.append("| Metric | Value |")
    lines.append("|--------|------:|")
    lines.append(f"| Total lines | {result['total_lines']} |")
    normal = result["total_lines"] - result["triage_lines"]
    lines.append(f"| Triage / non-success lines (below) | {result['triage_lines']} |")
    lines.append(f"| Normal startup success (`>> Loaded`, headers, banner) | {normal} |")
    lines.append(f"| **Unique message types (sections below)** | **{result['unique_section_count']}** |")
    lines.append("")
    lines.append(
        "Regenerate after a new run: "
        "`python scripts/parse-server-log-inventory.py <path/to/Server.log>` "
        "(writes `server-log-inventory.json` beside the log).\n"
    )
    lines.append("---\n")
    lines.append("## Message type catalog\n")
    lines.append(
        "One section per **unique message pattern** — not per spell id, opcode "
        "instance, or empty table row. Counts and instance lists are from the "
        "2026-06-27 sample log.\n"
    )
    lines.append("### Index\n")
    for i, sec in enumerate(result["sections"], 1):
        sid = sec["section_id"]
        lines.append(
            f"{i}. [{sec['title']}](#{sid}) — **{sec['count']}** lines, {sec['severity']}"
        )
    lines.append("\n---\n")

    current_group = None
    groups = {
        "spell_": "Spell script validation",
        "db_table": "Database (informational)",
        "invalid_battleground": "Startup config / DBC",
        "battleground_": "Startup config / DBC",
        "arena_season": "Startup config / DBC",
        "opcode_handler": "Startup config / DBC",
        "unhandled_opcode": "Runtime — opcodes",
        "packet_reenqueue": "Runtime — opcodes",
        "bytebuffer": "Runtime — packet parse",
        "client_service": "Runtime — client services",
        "world_debug": "Runtime — debug / log noise",
        "calendar_": "Runtime — debug / log noise",
        "session_": "Runtime — session flow (normal)",
        "player_init": "Runtime — session flow (normal)",
    }

    def group_for(sid: str) -> str:
        for prefix, label in groups.items():
            if sid.startswith(prefix):
                return label
        return "Other"

    for sec in result["sections"]:
        sid = sec["section_id"]
        grp = group_for(sid)
        if grp != current_group:
            current_group = grp
            lines.append(f"### {grp}\n")

        lines.append(f"#### {sec['title']} {{#{sid}}}\n")
        lines.append(f"- **Section id:** `{sid}`")
        lines.append(f"- **Lines in sample log:** {sec['count']}")
        lines.append(f"- **Severity:** {sec['severity']}")
        lines.append(f"- **Pattern:** `{sec['pattern']}`")
        lines.append(f"- **Fix / action:** {sec['fix']}")

        if sec.get("unique_spells"):
            lines.append(f"- **Unique spell ids:** {sec['unique_spells']}")
        if sec.get("unique_scripts"):
            lines.append(f"- **Unique script names:** {sec['unique_scripts']}")
        if sec.get("spell_ids_sample"):
            sample = ", ".join(str(x) for x in sec["spell_ids_sample"])
            suffix = " …" if sec.get("unique_spells", 0) > len(sec["spell_ids_sample"]) else ""
            lines.append(f"- **Spell id sample:** {sample}{suffix}")
        if sec.get("script_names_sample"):
            lines.append(
                "- **Script sample:** "
                + ", ".join(f"`{n}`" for n in sec["script_names_sample"])
                + (" …" if sec.get("unique_scripts", 0) > len(sec["script_names_sample"]) else "")
            )
        if sec.get("tables"):
            lines.append("- **Empty tables:** " + ", ".join(f"`{t}`" for t in sec["tables"]))
        if sec.get("opcodes"):
            lines.append("- **Opcodes seen:**")
            for op in sec["opcodes"]:
                lines.append(f"  - `{op}`")
        if sec.get("map_ids"):
            lines.append(f"- **Map ids:** {', '.join(str(x) for x in sec['map_ids'])}")
        if sec.get("bg_ids"):
            lines.append(f"- **Battleground ids:** {', '.join(str(x) for x in sec['bg_ids'])}")
        if sid == "bytebuffer_parse_exception":
            lines.append(
                "- **Fork note:** `CMSG_SETUP_WARBAND_GROUPS` is **fork warbands work**, "
                "not upstream drift — see "
                "[`warbands/char-select-campsites-handoff.md`](warbands/char-select-campsites-handoff.md)."
            )

        lines.append("- **Sample:**")
        for sample in sec.get("sample_lines", [])[:2]:
            lines.append(f"  ```text")
            lines.append(f"  {sample}")
            lines.append(f"  ```")
        lines.append("")

    return "\n".join(lines) + "\n"


def main() -> int:
    log_path = Path(sys.argv[1] if len(sys.argv) > 1 else "")
    if not log_path.is_file():
        print(f"Usage: {sys.argv[0]} <Server.log>", file=sys.stderr)
        return 1

    sections: dict[str, dict] = defaultdict(
        lambda: {
            "count": 0,
            "instance_spells": set(),
            "instance_scripts": set(),
            "instance_tables": set(),
            "instance_opcodes": set(),
            "instance_maps": set(),
            "instance_bg_ids": set(),
            "sample_lines": [],
        }
    )

    unclassified: list[str] = []
    total_lines = 0
    triage_lines = 0

    for line in log_path.read_text(encoding="utf-8", errors="replace").splitlines():
        total_lines += 1
        item = classify(line)
        if item is None:
            continue
        triage_lines += 1
        sid = item["section_id"]
        sec = sections[sid]
        sec["count"] += 1
        if len(sec["sample_lines"]) < 2:
            sec["sample_lines"].append(line)

        for key, dest in (
            ("instance_spell", "instance_spells"),
            ("instance_script", "instance_scripts"),
            ("instance_table", "instance_tables"),
            ("instance_opcode", "instance_opcodes"),
            ("instance_map", "instance_maps"),
            ("instance_bg_id", "instance_bg_ids"),
        ):
            if key in item and item[key] is not None:
                sec[dest].add(item[key])

    # Serialize sets
    out_sections = []
    for sid in sorted(sections.keys(), key=lambda s: (-sections[s]["count"], s)):
        data = sections[sid]
        meta = SECTION_META.get(sid)
        if meta is None and sid.startswith("spell_dbc_mismatch_"):
            meta = dbc_mismatch_meta(sid)
        if meta is None:
            meta = {"title": sid, "pattern": "?", "severity": "?", "fix": "?"}

        entry = {
            "section_id": sid,
            "count": data["count"],
            **meta,
            "sample_lines": data["sample_lines"],
        }
        if data["instance_spells"]:
            entry["unique_spells"] = len(data["instance_spells"])
            entry["spell_ids_sample"] = sorted(data["instance_spells"])[:10]
        if data["instance_scripts"]:
            entry["unique_scripts"] = len(data["instance_scripts"])
            entry["script_names_sample"] = sorted(data["instance_scripts"])[:10]
        if data["instance_tables"]:
            entry["tables"] = sorted(data["instance_tables"])
        if data["instance_opcodes"]:
            entry["opcodes"] = sorted(data["instance_opcodes"])
        if data["instance_maps"]:
            entry["map_ids"] = sorted(data["instance_maps"])
        if data["instance_bg_ids"]:
            entry["bg_ids"] = sorted(data["instance_bg_ids"])
        out_sections.append(entry)

    result = {
        "log_path": str(log_path),
        "total_lines": total_lines,
        "triage_lines": triage_lines,
        "unique_section_count": len(out_sections),
        "sections": out_sections,
        "unclassified_lines": unclassified,
    }

    out_json = log_path.parent / "server-log-inventory.json"
    out_json.write_text(json.dumps(result, indent=2), encoding="utf-8")

    md_path = Path(__file__).resolve().parents[1] / "docs" / "midnight-assessment" / "_server-log-catalog.generated.md"
    md_path.write_text(render_markdown_catalog(result), encoding="utf-8")

    print(json.dumps({k: result[k] for k in ("total_lines", "triage_lines", "unique_section_count")}, indent=2))
    print(f"Wrote {out_json}")
    print(f"Wrote {md_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
