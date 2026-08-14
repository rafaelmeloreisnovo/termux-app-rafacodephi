#!/usr/bin/env python3
"""RAFAELIA GitHub Actions control-plane inventory.

Stdlib-only scanner for every .github/workflows/*.yml|*.yaml file.
It does not rewrite or execute workflows. It makes orchestration state explicit,
keeps unknowns as TOKEN_VAZIO, and can be promoted to strict fail-closed mode.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import re
from pathlib import Path
from typing import Any

ROOT = Path(".github/workflows")
TOKEN_VAZIO = "TOKEN_VAZIO"
ALLOWED_TRACKS = {"debug", "internal", "official", "ops", "deprecated"}
TRIGGERS = ("workflow_dispatch", "workflow_call", "pull_request", "push", "schedule", "release", "repository_dispatch", "workflow_run")


def sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as fh:
        for chunk in iter(lambda: fh.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()


def header_value(text: str, key: str) -> str:
    match = re.search(rf"^#\s*{re.escape(key)}:\s*(.+?)\s*$", text, re.MULTILINE)
    return match.group(1).strip() if match else TOKEN_VAZIO


def top_level_name(text: str) -> str:
    for line in text.splitlines():
        if line.startswith("name:"):
            value = line.split(":", 1)[1].strip().strip('"\'')
            return value or TOKEN_VAZIO
    return TOKEN_VAZIO


def present_key(text: str, key: str) -> bool:
    return re.search(rf"(?m)^\s{{0,2}}{re.escape(key)}\s*:", text) is not None


def detect_triggers(text: str) -> list[str]:
    found: list[str] = []
    for trigger in TRIGGERS:
        if re.search(rf"(?m)^\s{{2,4}}{re.escape(trigger)}\s*:", text):
            found.append(trigger)
    return found


def classify(path: Path, text: str) -> dict[str, Any]:
    track = header_value(text, "ci_track")
    abis = header_value(text, "ci_abis")
    triggers = detect_triggers(text)
    name = top_level_name(text)
    manual = "workflow_dispatch" in triggers
    callable_ = "workflow_call" in triggers

    if track == "deprecated":
        role = "legacy-compatibility"
    elif callable_:
        role = "orchestratable"
    elif manual:
        role = "specialist-manual"
    else:
        role = "autonomous-specialist"

    return {
        "path": path.as_posix(),
        "sha256": sha256(path),
        "bytes": path.stat().st_size,
        "name": name,
        "ci_track": track,
        "ci_abis": abis,
        "role": role,
        "triggers": triggers,
        "has_permissions": present_key(text, "permissions"),
        "has_concurrency": present_key(text, "concurrency"),
        "has_timeout": "timeout-minutes:" in text,
        "workflow_dispatch": manual,
        "workflow_call": callable_,
    }


def validate(rows: list[dict[str, Any]], strict: bool) -> tuple[list[str], list[str]]:
    errors: list[str] = []
    warnings: list[str] = []
    names: dict[str, str] = {}

    for row in rows:
        path = row["path"]
        name = row["name"]
        track = row["ci_track"]
        abis = row["ci_abis"]

        if name == TOKEN_VAZIO:
            errors.append(f"{path}: missing top-level name")
        elif name in names:
            warnings.append(f"{path}: duplicate display name with {names[name]}: {name}")
        else:
            names[name] = path

        if track == TOKEN_VAZIO:
            (errors if strict else warnings).append(f"{path}: missing # ci_track")
        elif track not in ALLOWED_TRACKS:
            errors.append(f"{path}: invalid ci_track={track}")

        if abis == TOKEN_VAZIO:
            (errors if strict else warnings).append(f"{path}: missing # ci_abis")

        if track != "deprecated":
            if not row["has_permissions"]:
                warnings.append(f"{path}: permissions not explicit")
            if row["workflow_dispatch"] and not row["has_concurrency"]:
                warnings.append(f"{path}: manual workflow has no concurrency policy")
            if not row["has_timeout"]:
                warnings.append(f"{path}: no explicit timeout-minutes")

    return errors, warnings


def markdown(rows: list[dict[str, Any]], errors: list[str], warnings: list[str], strict: bool) -> str:
    counts: dict[str, int] = {}
    for row in rows:
        counts[row["role"]] = counts.get(row["role"], 0) + 1

    lines = [
        "# RAFAELIA Workflow Control Plane",
        "",
        f"- workflows discovered: **{len(rows)}**",
        f"- strict mode: **{'ON' if strict else 'OFF'}**",
        f"- errors: **{len(errors)}**",
        f"- warnings: **{len(warnings)}**",
        "- unknown/missing metadata remains **TOKEN_VAZIO**; it is never promoted to PASS implicitly.",
        "",
        "## Roles",
    ]
    for role in sorted(counts):
        lines.append(f"- `{role}`: {counts[role]}")

    lines.extend([
        "",
        "## Inventory",
        "",
        "| Workflow | Track | ABIs | Role | Dispatch | Call | Permissions | Concurrency | Timeout |",
        "|---|---|---|---|---:|---:|---:|---:|---:|",
    ])
    for row in rows:
        lines.append(
            f"| `{row['path']}` | `{row['ci_track']}` | `{row['ci_abis']}` | `{row['role']}` | "
            f"{'✓' if row['workflow_dispatch'] else '—'} | {'✓' if row['workflow_call'] else '—'} | "
            f"{'✓' if row['has_permissions'] else '—'} | {'✓' if row['has_concurrency'] else '—'} | "
            f"{'✓' if row['has_timeout'] else '—'} |"
        )

    if errors:
        lines.extend(["", "## Errors"])
        lines.extend(f"- ❌ {item}" for item in errors)
    if warnings:
        lines.extend(["", "## Migration warnings"])
        lines.extend(f"- ⚠️ {item}" for item in warnings)

    lines.extend([
        "",
        "## Contract",
        "",
        "`workflow discovered → classified → orchestratable/specialist/deprecated → evidence → claim`",
        "",
        "A workflow being discovered or callable is not execution evidence. Device validation remains a separate physical gate.",
    ])
    return "\n".join(lines) + "\n"


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--strict", action="store_true", help="fail on missing ci_track/ci_abis")
    parser.add_argument("--json", default="reports/workflow-control-plane.json")
    parser.add_argument("--markdown", default="reports/workflow-control-plane.md")
    args = parser.parse_args()

    if not ROOT.is_dir():
        raise SystemExit(f"workflow directory not found: {ROOT}")

    paths = sorted([*ROOT.glob("*.yml"), *ROOT.glob("*.yaml")])
    if not paths:
        raise SystemExit("no workflow files discovered")

    rows = [classify(path, path.read_text(encoding="utf-8")) for path in paths]
    errors, warnings = validate(rows, args.strict)

    payload = {
        "schema": "rafaelia.workflow-control-plane/v1",
        "state": "BLOCKED" if errors else ("OBSERVED_WITH_WARNINGS" if warnings else "PASS"),
        "claim_allowed": False,
        "strict": args.strict,
        "workflow_count": len(rows),
        "error_count": len(errors),
        "warning_count": len(warnings),
        "errors": errors,
        "warnings": warnings,
        "workflows": rows,
    }

    json_path = Path(args.json)
    md_path = Path(args.markdown)
    json_path.parent.mkdir(parents=True, exist_ok=True)
    md_path.parent.mkdir(parents=True, exist_ok=True)
    json_path.write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    md_path.write_text(markdown(rows, errors, warnings, args.strict), encoding="utf-8")

    print(f"workflow_count={len(rows)}")
    print(f"state={payload['state']}")
    print(f"errors={len(errors)} warnings={len(warnings)}")
    print(f"json={json_path} markdown={md_path}")
    return 1 if errors else 0


if __name__ == "__main__":
    raise SystemExit(main())
