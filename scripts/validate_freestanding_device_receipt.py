#!/usr/bin/env python3
"""Validate and reproduce RAFCODEPHI physical freestanding-runtime receipts.

Single receipts prove at most DEVICE_PROVEN. REPRODUCED requires two distinct,
independently valid receipts bound to the same gate, ABI, phase and candidate APK.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import re
import sys
from pathlib import Path
from typing import Any

SCHEMA = "raf.freestanding-runtime-evidence.v1"
SHA256_RE = re.compile(r"^[0-9a-f]{64}$")
ID_RE = re.compile(r"^[A-Za-z0-9._:-]{8,160}$")
ARCHES = {"aarch64", "armv7", "arm", "unknown"}
PHASES = {"probe", "bootstrap", "vectras", "full"}
BASE_CHECKS = ("gate_probe", "pkg", "proot", "proot_distro", "ninja", "clang", "cmake")
QEMU_CHECKS = ("qemu_system_x86_64", "qemu_img")
ALL_CHECKS = BASE_CHECKS + QEMU_CHECKS
TOP_KEYS = {
    "schema", "receipt_id", "created_unix", "architecture", "prefix", "gate_sha256",
    "candidate_apk_sha256", "phase", "runtime_state", "device_state",
    "reproduced_state", "claim_allowed", "checks",
}
CHECK_KEYS = {"required", "exit_code", "state"}


def sha256_file(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def error(errors: list[str], message: str) -> None:
    errors.append(message)


def validate_check(name: str, raw: Any, errors: list[str]) -> dict[str, Any] | None:
    if not isinstance(raw, dict):
        error(errors, f"checks.{name}: must be object")
        return None
    if set(raw) != CHECK_KEYS:
        error(errors, f"checks.{name}: exact keys required {sorted(CHECK_KEYS)}")
    required = raw.get("required")
    exit_code = raw.get("exit_code")
    state = raw.get("state")
    if not isinstance(required, bool):
        error(errors, f"checks.{name}.required: boolean required")
    if not isinstance(exit_code, int) or isinstance(exit_code, bool) or not (-1 <= exit_code <= 255):
        error(errors, f"checks.{name}.exit_code: integer -1..255 required")
    if state not in {"PASS", "TOKEN_VAZIO", "NOT_SELECTED"}:
        error(errors, f"checks.{name}.state: invalid state")
    if state == "PASS" and exit_code != 0:
        error(errors, f"checks.{name}: PASS requires exit_code=0")
    if state == "TOKEN_VAZIO" and exit_code == 0:
        error(errors, f"checks.{name}: TOKEN_VAZIO cannot have exit_code=0")
    if state == "NOT_SELECTED" and (required is not False or exit_code != -1):
        error(errors, f"checks.{name}: NOT_SELECTED requires required=false and exit_code=-1")
    return raw


def validate_receipt(path: Path, verify_sidecar: bool = True) -> dict[str, Any]:
    errors: list[str] = []
    try:
        raw_bytes = path.read_bytes()
        data = json.loads(raw_bytes.decode("utf-8"))
    except (OSError, UnicodeDecodeError, json.JSONDecodeError) as exc:
        return {"path": str(path), "valid": False, "errors": [f"read/json failure: {exc}"], "receipt": None}

    if not isinstance(data, dict):
        return {"path": str(path), "valid": False, "errors": ["top-level object required"], "receipt": None}
    if set(data) != TOP_KEYS:
        error(errors, f"top-level exact keys required {sorted(TOP_KEYS)}")
    if data.get("schema") != SCHEMA:
        error(errors, f"schema must be {SCHEMA}")
    receipt_id = data.get("receipt_id")
    if not isinstance(receipt_id, str) or not ID_RE.fullmatch(receipt_id):
        error(errors, "receipt_id invalid")
    created = data.get("created_unix")
    if not isinstance(created, int) or isinstance(created, bool) or created <= 0:
        error(errors, "created_unix must be positive integer")
    if data.get("architecture") not in ARCHES:
        error(errors, "architecture invalid")
    prefix = data.get("prefix")
    if not isinstance(prefix, str) or not prefix.startswith(("/data/data/", "/data/user/0/")) or not prefix.endswith("/files/usr"):
        error(errors, "prefix must be a private Android Termux prefix")
    gate_sha = data.get("gate_sha256")
    if not isinstance(gate_sha, str) or not SHA256_RE.fullmatch(gate_sha):
        error(errors, "gate_sha256 invalid")
    apk_sha = data.get("candidate_apk_sha256")
    if apk_sha is not None and (not isinstance(apk_sha, str) or not SHA256_RE.fullmatch(apk_sha)):
        error(errors, "candidate_apk_sha256 must be null or lowercase sha256")
    phase = data.get("phase")
    if phase not in PHASES:
        error(errors, "phase invalid")
    if data.get("runtime_state") not in {"RUNTIME_PROVEN", "TOKEN_VAZIO"}:
        error(errors, "runtime_state invalid")
    if data.get("device_state") not in {"DEVICE_PROVEN", "TOKEN_VAZIO"}:
        error(errors, "device_state invalid")
    if data.get("reproduced_state") not in {"REPRODUCED", "TOKEN_VAZIO"}:
        error(errors, "reproduced_state invalid")
    if not isinstance(data.get("claim_allowed"), bool):
        error(errors, "claim_allowed must be boolean")

    checks = data.get("checks")
    if not isinstance(checks, dict) or set(checks) != set(ALL_CHECKS):
        error(errors, f"checks must contain exactly {sorted(ALL_CHECKS)}")
        checks = checks if isinstance(checks, dict) else {}
    for name in ALL_CHECKS:
        validate_check(name, checks.get(name), errors)

    for name in BASE_CHECKS:
        item = checks.get(name)
        if isinstance(item, dict) and item.get("required") is not True:
            error(errors, f"checks.{name}: baseline check must be required")
    qemu_required = phase in {"vectras", "full"}
    for name in QEMU_CHECKS:
        item = checks.get(name)
        if not isinstance(item, dict):
            continue
        if item.get("required") is not qemu_required:
            error(errors, f"checks.{name}: required must be {str(qemu_required).lower()} for phase={phase}")
        if not qemu_required and item.get("state") != "NOT_SELECTED":
            error(errors, f"checks.{name}: non-QEMU phase must use NOT_SELECTED")

    selected_required_pass = all(
        isinstance(checks.get(name), dict)
        and (not checks[name].get("required") or checks[name].get("state") == "PASS")
        for name in ALL_CHECKS
    )
    promoted = (
        data.get("runtime_state") == "RUNTIME_PROVEN"
        and data.get("device_state") == "DEVICE_PROVEN"
        and data.get("claim_allowed") is True
    )
    if promoted != selected_required_pass:
        error(errors, "promotion invariant violated: claim/runtime/device must exactly match required PASS set")
    if data.get("runtime_state") == "RUNTIME_PROVEN" and data.get("device_state") != "DEVICE_PROVEN":
        error(errors, "RUNTIME_PROVEN physical receipt requires DEVICE_PROVEN")
    if data.get("reproduced_state") == "REPRODUCED":
        error(errors, "single physical receipt cannot self-promote REPRODUCED")

    if verify_sidecar:
        sidecar = Path(str(path) + ".sha256")
        if sidecar.exists():
            try:
                token = sidecar.read_text(encoding="utf-8").strip().split()[0]
            except (OSError, IndexError) as exc:
                error(errors, f"sha256 sidecar unreadable: {exc}")
            else:
                if token != sha256_file(path):
                    error(errors, "sha256 sidecar mismatch")

    return {"path": str(path), "valid": not errors, "errors": errors, "receipt": data}


def reproduce(first: dict[str, Any], second: dict[str, Any]) -> dict[str, Any]:
    errors: list[str] = []
    if not first.get("valid"):
        errors.append("first receipt invalid")
    if not second.get("valid"):
        errors.append("second receipt invalid")
    a = first.get("receipt") or {}
    b = second.get("receipt") or {}
    if a.get("receipt_id") == b.get("receipt_id"):
        errors.append("receipt_id must be distinct")
    if a.get("created_unix") == b.get("created_unix"):
        errors.append("created_unix must be distinct")
    for field in ("architecture", "gate_sha256", "phase"):
        if a.get(field) != b.get(field):
            errors.append(f"{field} mismatch")
    if a.get("candidate_apk_sha256") is None or b.get("candidate_apk_sha256") is None:
        errors.append("candidate_apk_sha256 required for REPRODUCED promotion")
    elif a.get("candidate_apk_sha256") != b.get("candidate_apk_sha256"):
        errors.append("candidate_apk_sha256 mismatch")
    for label, receipt in (("first", a), ("second", b)):
        if not (
            receipt.get("claim_allowed") is True
            and receipt.get("runtime_state") == "RUNTIME_PROVEN"
            and receipt.get("device_state") == "DEVICE_PROVEN"
        ):
            errors.append(f"{label} receipt is not DEVICE_PROVEN")
    return {
        "schema": "raf.freestanding-runtime-reproduction.v1",
        "receipt_ids": [a.get("receipt_id"), b.get("receipt_id")],
        "architecture": a.get("architecture") if a.get("architecture") == b.get("architecture") else None,
        "gate_sha256": a.get("gate_sha256") if a.get("gate_sha256") == b.get("gate_sha256") else None,
        "candidate_apk_sha256": a.get("candidate_apk_sha256") if a.get("candidate_apk_sha256") == b.get("candidate_apk_sha256") else None,
        "reproduced_state": "REPRODUCED" if not errors else "TOKEN_VAZIO",
        "claim_allowed": not errors,
        "errors": errors,
    }


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("receipts", nargs="+")
    parser.add_argument("--reproduce", action="store_true")
    parser.add_argument("--no-sidecar", action="store_true")
    parser.add_argument("--json", dest="json_path", type=Path)
    args = parser.parse_args(argv)
    if args.reproduce and len(args.receipts) != 2:
        parser.error("--reproduce requires exactly two receipts")

    results = [validate_receipt(Path(p), verify_sidecar=not args.no_sidecar) for p in args.receipts]
    if args.reproduce:
        payload: Any = reproduce(results[0], results[1])
        ok = payload["claim_allowed"]
    else:
        payload = {"schema": "raf.freestanding-runtime-validation.v1", "reports": results}
        ok = all(item["valid"] for item in results)
    rendered = json.dumps(payload, indent=2, sort_keys=True) + "\n"
    if args.json_path:
        args.json_path.parent.mkdir(parents=True, exist_ok=True)
        args.json_path.write_text(rendered, encoding="utf-8")
    sys.stdout.write(rendered)
    return 0 if ok else 1


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
