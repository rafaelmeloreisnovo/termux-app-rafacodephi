#!/usr/bin/env python3
"""Fail-closed validator for RAFCODEPHI physical-device E2E receipts.

A single receipt can prove source/build/device/workload observations, but it cannot
self-authorize reproducibility. Promotion requires a second receipt with matching
source/artifact hashes and workload output.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from datetime import datetime
from pathlib import Path

SCHEMA = "rafcodephi.e2e-device-receipt/v1"
REPO = "rafaelmeloreisnovo/termux-app-rafacodephi"
PACKAGE = "com.termux.rafacodephi"
PREFIX = "/data/data/com.termux.rafacodephi/files/usr"
ABIS = {"armeabi-v7a", "arm64-v8a"}
STATUS = {"PASS", "FAIL", "BLOCKED", "TOKEN_VAZIO"}
STAGES = ("packages", "bootstrap", "apk", "device", "workload", "receipt", "reproduction")
HEX = set("0123456789abcdef")


class ReceiptError(ValueError):
    pass


def is_hex(value: object, size: int) -> bool:
    return isinstance(value, str) and len(value) == size and all(c in HEX for c in value)


def canonical_receipt_id(doc: dict) -> str:
    clone = json.loads(json.dumps(doc))
    clone["receipt_id"] = "0" * 64
    payload = json.dumps(clone, sort_keys=True, separators=(",", ":"), ensure_ascii=False).encode()
    return hashlib.sha256(payload).hexdigest()


def validate_structure(doc: dict) -> list[str]:
    errors: list[str] = []
    if not isinstance(doc, dict):
        return ["receipt must be a JSON object"]
    required = {"schema", "receipt_id", "generated_at", "provenance", "device", "workload", "stages", "claim_allowed"}
    missing = sorted(required - set(doc))
    extras = sorted(set(doc) - required)
    if missing:
        errors.append(f"missing top-level keys: {', '.join(missing)}")
        return errors
    if extras:
        errors.append(f"unexpected top-level keys: {', '.join(extras)}")

    if doc["schema"] != SCHEMA:
        errors.append(f"schema must be {SCHEMA!r}")
    if not is_hex(doc["receipt_id"], 64):
        errors.append("receipt_id must be lowercase SHA-256")
    if not isinstance(doc["generated_at"], str) or len(doc["generated_at"]) < 20:
        errors.append("generated_at is missing/invalid")
    else:
        try:
            stamp = datetime.fromisoformat(doc["generated_at"].replace("Z", "+00:00"))
            if stamp.tzinfo is None or stamp.utcoffset() is None:
                errors.append("generated_at must include timezone offset")
        except ValueError:
            errors.append("generated_at must be ISO-8601")

    p = doc.get("provenance")
    if not isinstance(p, dict):
        errors.append("provenance must be an object")
    else:
        expected_p = {"repository", "git_commit", "build_receipt_sha256", "apk_sha256",
                      "bootstrap_source_repository", "bootstrap_source_commit",
                      "bootstrap_source_manifest_sha256", "bootstrap_sha256", "bootstrap_profile_sha256"}
        if set(p) != expected_p:
            errors.append("provenance keys are missing or unexpected")
        if p.get("repository") != REPO:
            errors.append(f"provenance.repository must be {REPO}")
        if p.get("bootstrap_source_repository") != "rafaelmeloreisnovo/termux-packages":
            errors.append("provenance.bootstrap_source_repository must be rafaelmeloreisnovo/termux-packages")
        for key in ("git_commit", "bootstrap_source_commit"):
            if not is_hex(p.get(key), 40):
                errors.append(f"provenance.{key} must be lowercase 40-hex")
        for key in (
            "build_receipt_sha256",
            "apk_sha256",
            "bootstrap_source_manifest_sha256",
            "bootstrap_sha256",
            "bootstrap_profile_sha256",
        ):
            if not is_hex(p.get(key), 64):
                errors.append(f"provenance.{key} must be lowercase SHA-256")

    d = doc.get("device")
    if not isinstance(d, dict):
        errors.append("device must be an object")
    else:
        expected_d = {"manufacturer", "model", "android_release", "abi", "package",
                      "prefix", "installed_apk_path", "installed_apk_sha256"}
        if set(d) != expected_d:
            errors.append("device keys are missing or unexpected")
        if d.get("package") != PACKAGE:
            errors.append(f"device.package must be {PACKAGE}")
        if d.get("prefix") != PREFIX:
            errors.append(f"device.prefix must be {PREFIX}")
        if d.get("abi") not in ABIS:
            errors.append(f"device.abi must be one of {sorted(ABIS)}")
        for key in ("manufacturer", "model", "android_release", "installed_apk_path"):
            if not isinstance(d.get(key), str) or not d.get(key):
                errors.append(f"device.{key} must be non-empty")
        if not is_hex(d.get("installed_apk_sha256"), 64):
            errors.append("device.installed_apk_sha256 must be lowercase SHA-256")

    w = doc.get("workload")
    if not isinstance(w, dict):
        errors.append("workload must be an object")
    else:
        if set(w) != {"command", "exit_code", "stdout_sha256"}:
            errors.append("workload keys are missing or unexpected")
        if not isinstance(w.get("command"), str) or not w.get("command"):
            errors.append("workload.command must be non-empty")
        if type(w.get("exit_code")) is not int:
            errors.append("workload.exit_code must be integer (not boolean)")
        if not is_hex(w.get("stdout_sha256"), 64):
            errors.append("workload.stdout_sha256 must be lowercase SHA-256")

    stages = doc.get("stages")
    if not isinstance(stages, dict):
        errors.append("stages must be an object")
    else:
        if set(stages) != set(STAGES):
            errors.append("stages must contain exactly: " + ", ".join(STAGES))
        for key in STAGES:
            if stages.get(key) not in STATUS:
                errors.append(f"stages.{key} has invalid status {stages.get(key)!r}")
        if stages.get("reproduction") != "TOKEN_VAZIO":
            errors.append("single device receipt cannot claim reproduction")

    if doc.get("claim_allowed") is not False:
        errors.append("single device receipt must set claim_allowed=false")

    if not errors and doc["receipt_id"] != canonical_receipt_id(doc):
        errors.append("receipt_id does not match canonical SHA-256 of receipt content")

    return errors


def same_observation(a: dict, b: dict) -> list[str]:
    mismatches: list[str] = []
    for key in (
        "repository",
        "git_commit",
        "build_receipt_sha256",
        "apk_sha256",
        "bootstrap_source_repository",
        "bootstrap_source_commit",
        "bootstrap_source_manifest_sha256",
        "bootstrap_sha256",
        "bootstrap_profile_sha256",
    ):
        if a["provenance"][key] != b["provenance"][key]:
            mismatches.append(f"provenance.{key}")
    if a["workload"]["command"] != b["workload"]["command"]:
        mismatches.append("workload.command")
    if a["workload"]["stdout_sha256"] != b["workload"]["stdout_sha256"]:
        mismatches.append("workload.stdout_sha256")
    if a["receipt_id"] == b["receipt_id"]:
        mismatches.append("receipt_id_not_independent")
    return mismatches


def promotion_state(receipt: dict, reference: dict | None) -> tuple[bool, str]:
    first_six = ("packages", "bootstrap", "apk", "device", "workload", "receipt")
    bad = [k for k in first_six if receipt["stages"][k] != "PASS"]
    if bad:
        return False, "current receipt has non-PASS stages: " + ", ".join(bad)
    if receipt["workload"]["exit_code"] != 0:
        return False, "current workload exit_code is non-zero"
    if receipt["device"]["installed_apk_sha256"] != receipt["provenance"]["apk_sha256"]:
        return False, "installed APK hash differs from build provenance"
    if reference is None:
        return False, "reproduction evidence absent (TOKEN_VAZIO)"
    ref_bad = [k for k in first_six if reference["stages"][k] != "PASS"]
    if ref_bad:
        return False, "reference receipt has non-PASS stages: " + ", ".join(ref_bad)
    if reference["workload"]["exit_code"] != 0:
        return False, "reference workload exit_code is non-zero"
    if reference["device"]["installed_apk_sha256"] != reference["provenance"]["apk_sha256"]:
        return False, "reference installed APK hash differs from build provenance"
    if reference.get("claim_allowed") is True:
        return False, "reference receipt attempted to self-authorize"
    mismatch = same_observation(receipt, reference)
    if mismatch:
        return False, "reproduction mismatch: " + ", ".join(mismatch)
    return True, "two independent receipts reproduce the same cross-repo build/workload observation"


MAX_RECEIPT_BYTES = 1024 * 1024


def reject_duplicate_keys(pairs: list[tuple[str, object]]) -> dict:
    doc = {}
    for key, value in pairs:
        if key in doc:
            raise ReceiptError(f"duplicate JSON key: {key}")
        doc[key] = value
    return doc


def load(path: Path) -> dict:
    try:
        if path.stat().st_size > MAX_RECEIPT_BYTES:
            raise ReceiptError(f"{path}: receipt exceeds 1 MiB limit")
        return json.loads(path.read_text(encoding="utf-8"), object_pairs_hook=reject_duplicate_keys)
    except (OSError, UnicodeError, json.JSONDecodeError) as exc:
        raise ReceiptError(f"{path}: {exc}") from exc


def emit_result(ok: bool, reason: str, receipt: dict | None = None) -> None:
    result = {
        "schema": "rafcodephi.e2e-promotion-result/v1",
        "claim_allowed": ok,
        "gate": "PASS" if ok else "BLOCKED",
        "reason": reason,
    }
    if receipt:
        result["receipt_id"] = receipt.get("receipt_id")
    print(json.dumps(result, sort_keys=True))


def self_test() -> int:
    base = {
        "schema": SCHEMA,
        "receipt_id": "0" * 64,
        "generated_at": "2026-08-08T17:00:00-03:00",
        "provenance": {
            "repository": REPO,
            "git_commit": "a" * 40,
            "build_receipt_sha256": "9" * 64,
            "apk_sha256": "b" * 64,
            "bootstrap_source_repository": "rafaelmeloreisnovo/termux-packages",
            "bootstrap_source_commit": "e" * 40,
            "bootstrap_source_manifest_sha256": "8" * 64,
            "bootstrap_sha256": "c" * 64,
            "bootstrap_profile_sha256": "7" * 64,
        },
        "device": {
            "manufacturer": "synthetic",
            "model": "fixture-a",
            "android_release": "15",
            "abi": "arm64-v8a",
            "package": PACKAGE,
            "prefix": PREFIX,
            "installed_apk_path": "/data/app/synthetic/base.apk",
            "installed_apk_sha256": "b" * 64,
        },
        "workload": {"command": "synthetic-workload", "exit_code": 0, "stdout_sha256": "d" * 64},
        "stages": {k: "PASS" for k in STAGES},
        "claim_allowed": False,
    }
    base["stages"]["reproduction"] = "TOKEN_VAZIO"
    base["receipt_id"] = canonical_receipt_id(base)
    if validate_structure(base):
        raise AssertionError(validate_structure(base))
    ok, _ = promotion_state(base, None)
    if ok:
        raise AssertionError("single receipt must never promote")

    second = json.loads(json.dumps(base))
    second["generated_at"] = "2026-08-08T17:05:00-03:00"
    second["device"]["model"] = "fixture-b"
    second["receipt_id"] = canonical_receipt_id(second)
    if validate_structure(second):
        raise AssertionError(validate_structure(second))
    ok, _ = promotion_state(second, base)
    if not ok:
        raise AssertionError("matching independent receipts should promote")

    drift = json.loads(json.dumps(second))
    drift["workload"]["stdout_sha256"] = "e" * 64
    drift["receipt_id"] = canonical_receipt_id(drift)
    ok, _ = promotion_state(drift, base)
    if ok:
        raise AssertionError("output drift must block promotion")

    print("SELF_TEST PASS: single=BLOCKED reproduced=PASS drift=BLOCKED")
    return 0


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("receipt", nargs="?")
    ap.add_argument("--reference")
    ap.add_argument("--self-test", action="store_true")
    args = ap.parse_args()

    if args.self_test:
        return self_test()
    if not args.receipt:
        ap.error("receipt path is required unless --self-test is used")

    try:
        current = load(Path(args.receipt))
        errors = validate_structure(current)
        if errors:
            emit_result(False, "; ".join(errors), current)
            return 2

        reference = None
        if args.reference:
            reference = load(Path(args.reference))
            ref_errors = validate_structure(reference)
            if ref_errors:
                emit_result(False, "reference invalid: " + "; ".join(ref_errors), current)
                return 2

        ok, reason = promotion_state(current, reference)
        if current.get("claim_allowed") is True:
            emit_result(False, "receipt attempted to self-authorize claim_allowed=true", current)
            return 3
        emit_result(ok, reason, current)
        return 0 if ok else 4
    except ReceiptError as exc:
        emit_result(False, str(exc))
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
