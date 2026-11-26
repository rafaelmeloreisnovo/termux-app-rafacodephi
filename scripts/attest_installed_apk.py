#!/usr/bin/env python3
"""Read-only ADB APK identity attestation. Not an installation or E2E test.

Hosted Python/ADB boundary; does not claim to be a freestanding core.
Only bounded, sanitized evidence is written; no serials or private paths.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys
from datetime import datetime, timezone

HEX64 = re.compile(r"^[0-9a-f]{64}$")
DEFAULT_MANIFEST = Path(__file__).resolve().parents[1] / "configs/device-apk-attestation-42811514174.v1.json"


class Blocked(Exception):
    pass


def run(argv):
    try:
        proc = subprocess.run(argv, text=True, capture_output=True, timeout=15, check=False)
    except (OSError, subprocess.TimeoutExpired) as exc:
        raise Blocked("ADB_UNAVAILABLE_OR_TIMEOUT") from exc
    if proc.returncode:
        raise Blocked("ADB_COMMAND_FAILED")
    return proc.stdout.replace("\r", "")


def digest(path):
    h = hashlib.sha256()
    with open(path, "rb") as f:
        for block in iter(lambda: f.read(1024 * 1024), b""):
            h.update(block)
    return h.hexdigest()


def probe(manifest, variant, candidate, adb):
    if manifest.get("schema") != "rafcodephi.device-apk-attestation-manifest/v1":
        raise Blocked("MANIFEST_SCHEMA_DRIFT")
    if manifest.get("package") != "com.termux.rafacodephi":
        raise Blocked("PACKAGE_IDENTITY_DRIFT")
    target = manifest.get("variants", {}).get(variant)
    if not isinstance(target, dict):
        raise Blocked("VARIANT_NOT_DECLARED")
    expected = target.get("sha256", "")
    if not isinstance(expected, str) or not HEX64.fullmatch(expected):
        raise Blocked("EXPECTED_HASH_INVALID")
    if candidate:
        p = Path(candidate)
        if not p.is_file() or p.stat().st_size != target.get("bytes") or digest(p) != expected:
            raise Blocked("CANDIDATE_APK_MISMATCH")

    devices = run([adb, "devices"]).splitlines()
    live = [line.split()[0] for line in devices[1:]
            if len(line.split()) >= 2 and line.split()[1] == "device"]
    if len(live) != 1:
        raise Blocked("EXACTLY_ONE_AUTHORIZED_DEVICE_REQUIRED")
    serial = live[0]  # Never written to receipt or stdout.
    base = [adb, "-s", serial, "shell"]
    abi = run(base + ["getprop", "ro.product.cpu.abi"]).strip()
    if abi not in target.get("allowed_device_abis", []):
        raise Blocked("ABI_MISMATCH")
    package_paths = run(base + ["pm", "path", manifest["package"]]).splitlines()
    if len(package_paths) != 1 or not package_paths[0].startswith("package:"):
        raise Blocked("SPLIT_OR_MISSING_APK_PATH")
    apk_path = package_paths[0][8:].strip()
    if not apk_path.startswith("/") or not apk_path.endswith("/base.apk"):
        raise Blocked("BASE_APK_PATH_INVALID")
    output = run(base + ["sha256sum", apk_path]).strip()
    match = re.fullmatch(r"([0-9a-fA-F]{64})\s+(\S+)", output)
    if not match or match.group(2) != apk_path:
        raise Blocked("DEVICE_HASH_EVIDENCE_INVALID")
    observed = match.group(1).lower()
    if observed != expected:
        raise Blocked("INSTALLED_APK_HASH_MISMATCH")
    return abi, observed


def save_new(path, receipt):
    # Exclusive creation prevents silent overwrite of prior evidence.
    p = Path(path)
    p.parent.mkdir(parents=True, exist_ok=True)
    with p.open("x", encoding="utf-8") as f:
        json.dump(receipt, f, indent=2, sort_keys=True)
        f.write("\n")
        f.flush()
        os.fsync(f.fileno())


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--manifest", type=Path, default=DEFAULT_MANIFEST)
    parser.add_argument("--variant", required=True, choices=("armeabi-v7a", "arm64-v8a", "universal"))
    parser.add_argument("--candidate-apk", type=Path, help="Optional offline candidate byte check")
    parser.add_argument("--receipt", type=Path, required=True, help="New receipt filename; existing files rejected")
    parser.add_argument("--adb", default="adb", help="ADB executable, for controlled tests")
    args = parser.parse_args(argv)
    state, reason, abi, observed = "BLOCKED", "UNOBSERVED", "TOKEN_VAZIO", "TOKEN_VAZIO"
    expected = "TOKEN_VAZIO"
    ci_run = "TOKEN_VAZIO"
    try:
        manifest = json.loads(args.manifest.read_text(encoding="utf-8"))
        ci_run = manifest.get("ci_run_id", "TOKEN_VAZIO")
        expected = manifest.get("variants", {}).get(args.variant, {}).get("sha256", "TOKEN_VAZIO")
        abi, observed = probe(manifest, args.variant, args.candidate_apk, args.adb)
        state, reason = "PASS_SCOPED", "INSTALLED_BASE_APK_BYTES_MATCH"
    except Blocked as exc:
        reason = str(exc)
    except (OSError, ValueError, TypeError, KeyError):
        reason = "MANIFEST_OR_IO_INVALID"
    receipt = {
        "schema": "rafcodephi.device-apk-attestation/v1",
        "timestamp_utc": datetime.now(timezone.utc).isoformat(),
        "ci_run_id": ci_run,
        "package": "com.termux.rafacodephi",
        "variant": args.variant,
        "device_abi": abi,
        "expected_apk_sha256": expected,
        "observed_installed_base_apk_sha256": observed,
        "state": state,
        "reason": reason,
        "install_performed": False,
        "claim_allowed_release": False,
        "claim_allowed_benchmark_binding": False,
        "apt_pkg_proot_e2e": "TOKEN_VAZIO",
        "signing_validation": "NOT_RUN",
        "source_commit_binding": "TOKEN_VAZIO",
    }
    try:
        save_new(args.receipt, receipt)
    except (OSError, ValueError) as exc:
        print(f"BLOCKED: receipt write failed ({type(exc).__name__})", file=sys.stderr)
        return 3
    print(f"{state} reason={reason} receipt={args.receipt}")
    return 0 if state == "PASS_SCOPED" else 2


if __name__ == "__main__":
    sys.exit(main())
