#!/usr/bin/env python3
"""Validate legacy runtime evidence and the v2 bounded device receipt."""
from __future__ import annotations

import argparse
import hashlib
import json
import re
import sys
from pathlib import Path
from typing import Any

TOKEN_VAZIO = "TOKEN_VAZIO"
SCHEMA_V1 = "termux.rafacodephi.runtime_evidence.v1"
SCHEMA_V2 = "termux.rafacodephi.runtime_receipt.v2"
REPOSITORY = "rafaelmeloreisnovo/termux-app-rafacodephi"
ALLOWED_BACKEND = {
    TOKEN_VAZIO,
    "PRESENT_UNVERIFIED",
    "WRAPPER_ONLY_OR_INCOMPLETE",
}
ALLOWED_STATES = {
    "HOST_SIMULATION",
    "DEVICE_OBSERVED_INCOMPLETE",
    "DEVICE_RECEIPT_COMPLETE",
}
HEX40 = re.compile(r"^[0-9a-f]{40}$")
HEX64 = re.compile(r"^[0-9a-f]{64}$")
REQUIRED_COMMANDS = {"sh", "ls", "pkg", "apt", "dpkg", "proot"}


def canonical_digest(data: dict[str, Any]) -> str:
    material = dict(data)
    material.pop("receipt_sha256", None)
    encoded = json.dumps(
        material,
        ensure_ascii=False,
        sort_keys=True,
        separators=(",", ":"),
    ).encode("utf-8")
    return hashlib.sha256(encoded).hexdigest()


def _legacy_validate(data: dict[str, Any]) -> list[str]:
    errors: list[str] = []
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
    if apk_sha != TOKEN_VAZIO and (
        not isinstance(apk_sha, str) or HEX64.fullmatch(apk_sha) is None
    ):
        errors.append("invalid apk sha256")
    commands = data.get("commands")
    names = {
        item.get("name")
        for item in commands if isinstance(commands, list) and isinstance(item, dict)
    }
    if not isinstance(commands, list) or names != REQUIRED_COMMANDS:
        errors.append("complete command probes required")
    for item in commands if isinstance(commands, list) else []:
        if not isinstance(item, dict) or not isinstance(
            item.get("probe_exit_code"), int
        ):
            label = item.get("name") if isinstance(item, dict) else "command"
            errors.append(f"{label}: exit code required")
    if not data.get("next_action") or not data.get("rollback"):
        errors.append("next_action and rollback required")
    return errors


def validate(data: dict[str, Any]) -> list[str]:
    schema = data.get("schema")
    if schema == SCHEMA_V1:
        return _legacy_validate(data)

    errors: list[str] = []
    if schema != SCHEMA_V2:
        return ["invalid schema"]
    if data.get("version") != 2:
        errors.append("version must be 2")
    if (
        data.get("authority") != REPOSITORY
        or data.get("producer_repository") != REPOSITORY
    ):
        errors.append("unexpected repository authority")
    if data.get("adapter_state") != "IMPLEMENTED":
        errors.append("adapter_state must be IMPLEMENTED")
    if data.get("claim_allowed") is not False:
        errors.append("claim_allowed must be false")
    if data.get("install_or_mutation_performed") is not False:
        errors.append("collector must be non-destructive")

    state = data.get("evidence_state")
    if state not in ALLOWED_STATES:
        errors.append("invalid evidence_state")

    commit = data.get("producer_commit")
    if commit != TOKEN_VAZIO and (
        not isinstance(commit, str) or HEX40.fullmatch(commit) is None
    ):
        errors.append("producer_commit must be TOKEN_VAZIO or a lowercase git SHA")

    apk_sha = data.get("apk_sha256")
    if apk_sha != TOKEN_VAZIO and (
        not isinstance(apk_sha, str) or HEX64.fullmatch(apk_sha) is None
    ):
        errors.append("apk_sha256 must be TOKEN_VAZIO or sha256")

    commands = data.get("commands")
    names = {
        item.get("name")
        for item in commands if isinstance(commands, list) and isinstance(item, dict)
    }
    if not isinstance(commands, list) or names != REQUIRED_COMMANDS:
        errors.append("complete command probes required")
    for item in commands if isinstance(commands, list) else []:
        if not isinstance(item, dict):
            errors.append("command record must be an object")
            continue
        if not isinstance(item.get("probe_exit_code"), int):
            errors.append(f"{item.get('name')}: exit code required")
        for key in ("stdout_sha256", "stderr_sha256"):
            value = item.get(key)
            if value != TOKEN_VAZIO and (
                not isinstance(value, str) or HEX64.fullmatch(value) is None
            ):
                errors.append(f"{item.get('name')}: invalid {key}")

    device = data.get("device")
    if not isinstance(device, dict):
        errors.append("device object required")
        device = {}
    android_target = (
        device.get("model") != TOKEN_VAZIO
        and device.get("abi_primary") != TOKEN_VAZIO
    )

    termux_info = data.get("termux_info")
    if not isinstance(termux_info, dict):
        errors.append("termux_info object required")
        termux_info = {}
    termux_hash = termux_info.get("stdout_sha256")
    if termux_hash != TOKEN_VAZIO and (
        not isinstance(termux_hash, str) or HEX64.fullmatch(termux_hash) is None
    ):
        errors.append("invalid termux-info sha256")

    complete = bool(
        android_target
        and isinstance(commit, str)
        and HEX40.fullmatch(commit)
        and isinstance(apk_sha, str)
        and HEX64.fullmatch(apk_sha)
        and data.get("prefix") != TOKEN_VAZIO
        and termux_info.get("present") is True
        and termux_info.get("exit_code") == 0
        and isinstance(commands, list)
        and all(
            next(
                (
                    item.get("exists") and item.get("executable")
                    for item in commands
                    if item.get("name") == name
                ),
                False,
            )
            for name in ("sh", "ls")
        )
    )
    expected_state = (
        "DEVICE_RECEIPT_COMPLETE"
        if complete
        else "DEVICE_OBSERVED_INCOMPLETE"
        if android_target
        else "HOST_SIMULATION"
    )
    if state != expected_state:
        errors.append(f"evidence_state must derive to {expected_state}")
    if data.get("execution_evidence_usable") is not complete:
        errors.append("execution_evidence_usable must match completeness derivation")

    digest = data.get("receipt_sha256")
    if not isinstance(digest, str) or HEX64.fullmatch(digest) is None:
        errors.append("receipt_sha256 required")
    elif digest != canonical_digest(data):
        errors.append("receipt_sha256 mismatch")

    if not data.get("observed_at") or not data.get("package_name"):
        errors.append("observed_at and package_name required")
    if not data.get("next_action") or not data.get("rollback"):
        errors.append("next_action and rollback required")
    return errors


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("evidence", type=Path)
    parser.add_argument("--require-device-complete", action="store_true")
    args = parser.parse_args()
    try:
        data = json.loads(args.evidence.read_text(encoding="utf-8"))
        if not isinstance(data, dict):
            raise ValueError("root must be an object")
        errors = validate(data)
        if (
            args.require_device_complete
            and data.get("evidence_state") != "DEVICE_RECEIPT_COMPLETE"
        ):
            errors.append("target device receipt is not complete")
    except Exception as exc:
        print(f"BLOCKED: {exc}", file=sys.stderr)
        return 2
    print(
        json.dumps(
            {
                "status": "PASS" if not errors else "FAIL",
                "schema": data.get("schema"),
                "evidence_state": data.get(
                    "evidence_state", data.get("runtime_status")
                ),
                "errors": errors,
                "claim_allowed": False,
            },
            indent=2,
            sort_keys=True,
        )
    )
    return 0 if not errors else 1


if __name__ == "__main__":
    raise SystemExit(main())
