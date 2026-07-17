#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

ALLOWED_BACKEND = {"TOKEN_VAZIO", "PRESENT_UNVERIFIED", "WRAPPER_ONLY_OR_INCOMPLETE"}


def validate(data: dict) -> list[str]:
    errors: list[str] = []
    if data.get("schema") != "termux.rafacodephi.runtime_evidence.v1":
        errors.append("invalid schema")
    if data.get("claim_allowed") is not False:
        errors.append("claim_allowed must be false")
    if data.get("install_or_mutation_performed") is not False:
        errors.append("collector must be non-destructive")
    if not data.get("device_abi"):
        errors.append("device_abi required")
    if not data.get("prefix"):
        errors.append("prefix required")
    if data.get("package_backend_status") not in ALLOWED_BACKEND:
        errors.append("invalid package backend status")
    apk_sha = data.get("apk_sha256")
    if apk_sha != "TOKEN_VAZIO" and (not isinstance(apk_sha, str) or len(apk_sha) != 64):
        errors.append("invalid apk sha256")

    commands = data.get("commands")
    required = {"sh", "ls", "pkg", "apt", "dpkg", "proot"}
    if not isinstance(commands, list) or {item.get("name") for item in commands} != required:
        errors.append("complete command probes required")
    for item in commands if isinstance(commands, list) else []:
        if not isinstance(item.get("probe_exit_code"), int):
            errors.append(f"{item.get('name')}: exit code required")
    if not data.get("next_action") or not data.get("rollback"):
        errors.append("next_action and rollback required")
    return errors


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("evidence", type=Path)
    args = parser.parse_args()
    try:
        data = json.loads(args.evidence.read_text(encoding="utf-8"))
        errors = validate(data)
    except Exception as exc:
        print(f"BLOCKED: {exc}", file=sys.stderr)
        return 2
    print(json.dumps({"status": "PASS" if not errors else "FAIL", "errors": errors, "claim_allowed": False}, indent=2))
    return 0 if not errors else 1


if __name__ == "__main__":
    raise SystemExit(main())
