#!/usr/bin/env python3
"""Validate the RAFAELIA ZERO ARM32+ARM64 physical-device evidence matrix."""

from __future__ import annotations

import argparse
import copy
import importlib.util
import json
import os
import pathlib
import sys
import tempfile
from typing import Any

ROOT = pathlib.Path(__file__).resolve().parents[1]
BUNDLE_VALIDATOR = ROOT / "scripts/validate_rafaelia_zero_device_bundle.py"
BUNDLE_BUILDER = ROOT / "scripts/create_rafaelia_zero_device_bundle.py"
RECEIPT_VALIDATOR = ROOT / "scripts/validate_rafaelia_zero_device_receipt.py"
MATRIX_SCHEMA = "rafaelia.zero.device.evidence-matrix.v1"
REQUIRED_ROLES = ("arm32-legacy", "arm64-modern")
OPTIONAL_ROLES = ("x86", "x86_64")


class MatrixValidationError(ValueError):
    pass


def require(condition: bool, message: str) -> None:
    if not condition:
        raise MatrixValidationError(message)


def load_module(path: pathlib.Path, name: str):
    spec = importlib.util.spec_from_file_location(name, path)
    require(spec is not None and spec.loader is not None, f"cannot load {path.name}")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def write_atomic(path: pathlib.Path, text: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = path.with_name(path.name + ".tmp")
    with temporary.open("w", encoding="utf-8", newline="\n") as stream:
        stream.write(text)
        stream.flush()
        os.fsync(stream.fileno())
    os.replace(temporary, path)
    descriptor = os.open(path.parent, os.O_RDONLY)
    try:
        os.fsync(descriptor)
    finally:
        os.close(descriptor)


def validate_matrix(bundle_paths: list[pathlib.Path]) -> dict[str, Any]:
    bundle_validator = load_module(BUNDLE_VALIDATOR, "rafz_bundle_matrix")
    summaries: list[dict[str, Any]] = []
    roles: dict[str, dict[str, Any]] = {}
    receipt_hashes: set[str] = set()
    serials: set[str] = set()
    fingerprints: set[str] = set()

    for bundle_path in bundle_paths:
        summary = bundle_validator.validate_bundle(bundle_path)
        role = summary["role"]
        require(role in REQUIRED_ROLES + OPTIONAL_ROLES, f"unsupported role: {role}")
        require(role not in roles, f"duplicate role in canonical matrix: {role}")
        require(summary["receipt_sha256"] not in receipt_hashes,
                f"duplicate receipt digest: {summary['receipt_sha256']}")
        require(summary["device_serial"] not in serials,
                f"duplicate device serial: {summary['device_serial']}")
        require(summary["device_fingerprint"] not in fingerprints,
                f"duplicate device fingerprint: {summary['device_fingerprint']}")
        roles[role] = summary
        receipt_hashes.add(summary["receipt_sha256"])
        serials.add(summary["device_serial"])
        fingerprints.add(summary["device_fingerprint"])
        summaries.append(summary)

    observed_required = [role for role in REQUIRED_ROLES if role in roles]
    missing_required = [role for role in REQUIRED_ROLES if role not in roles]
    if len(observed_required) == 0:
        state = "TOKEN_VAZIO"
        claim_allowed = False
    elif len(observed_required) < len(REQUIRED_ROLES):
        state = "PARTIAL_DEVICE_PROOF"
        claim_allowed = False
    else:
        state = "DUAL_ARM_DEVICE_PROOF"
        claim_allowed = True

    return {
        "schema": MATRIX_SCHEMA,
        "result": "PASS",
        "state": state,
        "claim_allowed_device_matrix": claim_allowed,
        "release_claim_allowed": False,
        "required_roles": list(REQUIRED_ROLES),
        "observed_required_roles": observed_required,
        "missing_required_roles": missing_required,
        "optional_roles_observed": [role for role in OPTIONAL_ROLES if role in roles],
        "bundle_count": len(summaries),
        "bundles": sorted(summaries, key=lambda item: item["role"]),
        "limits": {
            "debug_apk_evidence_only": True,
            "independent_reproduction": "TOKEN_VAZIO",
            "release_promotion": False,
        },
    }


def write_source(builder, root: pathlib.Path, name: str, receipt: dict[str, Any], serial: str):
    source = root / name
    source.mkdir()
    capture = {
        "schema": "rafaelia.zero.device.capture.v1",
        "package": receipt["package"],
        "device_serial": serial,
        "device_fingerprint": receipt["device"]["fingerprint"],
        "installed_apk_path": f"/data/app/{serial}/base.apk",
        "captured_at_unix_ms": receipt["timestamp_unix_ms"],
    }
    receipt_path = source / "receipt.json"
    capture_path = source / "capture.json"
    transcript_path = source / "transcript.txt"
    apk_path = source / "probe.apk"
    receipt_path.write_text(builder.canonical_json(receipt), encoding="utf-8")
    capture_path.write_text(builder.canonical_json(capture), encoding="utf-8")
    apk_path.write_bytes(b"same-universal-debug-apk")
    transcript_path.write_text(
        "RAFAELIA_ZERO_DEVICE_PROBE=PASS\n"
        f"receipt_sha256={builder.sha256_file(receipt_path)}\n"
        f"apk_sha256={builder.sha256_file(apk_path)}\n",
        encoding="utf-8",
    )
    bundle = root / f"bundle-{name}"
    builder.build_bundle(receipt_path, capture_path, apk_path, transcript_path, bundle)
    return bundle


def self_test() -> None:
    builder = load_module(BUNDLE_BUILDER, "rafz_matrix_builder")
    receipt_validator = load_module(RECEIPT_VALIDATOR, "rafz_matrix_receipt")
    arm32 = receipt_validator.sample_receipt()
    arm32["device"]["fingerprint"] = "selftest/arm32/fingerprint"
    arm32["timestamp_unix_ms"] = 1784548800001

    arm64 = copy.deepcopy(arm32)
    arm64["timestamp_unix_ms"] = 1784548800002
    arm64["device"]["fingerprint"] = "selftest/arm64/fingerprint"
    arm64["device"]["process_arch"] = "aarch64"
    arm64["device"]["supported_abis"] = "arm64-v8a,armeabi-v7a"
    arm64["native"]["architecture_id"] = 2
    arm64["native"]["expected_architecture_id"] = 2
    arm64["observed"]["sequence"] = 84
    arm64["observed"]["digest_after"] = 3

    with tempfile.TemporaryDirectory(prefix="rafz-device-matrix-") as temporary:
        root = pathlib.Path(temporary)
        arm32_bundle = write_source(builder, root, "arm32", arm32, "SELFTEST-ARM32")
        arm64_bundle = write_source(builder, root, "arm64", arm64, "SELFTEST-ARM64")

        empty = validate_matrix([])
        require(empty["state"] == "TOKEN_VAZIO", "empty matrix state mismatch")
        require(empty["claim_allowed_device_matrix"] is False, "empty matrix promoted claim")

        partial = validate_matrix([arm32_bundle])
        require(partial["state"] == "PARTIAL_DEVICE_PROOF", "partial matrix state mismatch")
        require(partial["claim_allowed_device_matrix"] is False, "partial matrix promoted claim")

        complete = validate_matrix([arm32_bundle, arm64_bundle])
        require(complete["state"] == "DUAL_ARM_DEVICE_PROOF", "complete matrix state mismatch")
        require(complete["claim_allowed_device_matrix"] is True, "complete matrix did not promote claim")

        try:
            validate_matrix([arm32_bundle, arm32_bundle])
        except MatrixValidationError:
            pass
        else:
            raise AssertionError("duplicate bundle was accepted")

        output = root / "matrix.json"
        encoded = json.dumps(complete, sort_keys=True, indent=2, ensure_ascii=False) + "\n"
        write_atomic(output, encoded)
        require(output.read_text(encoding="utf-8") == encoded, "atomic matrix output mismatch")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("bundles", nargs="*", type=pathlib.Path)
    parser.add_argument("--self-test", action="store_true")
    parser.add_argument("--output", type=pathlib.Path)
    args = parser.parse_args()

    if args.self_test:
        try:
            self_test()
        except (AssertionError, MatrixValidationError, OSError, ValueError) as exc:
            print(f"RAFAELIA_ZERO_DEVICE_MATRIX_SELF_TEST=FAIL: {exc}", file=sys.stderr)
            return 1
        print("RAFAELIA_ZERO_DEVICE_MATRIX_SELF_TEST=PASS")
        return 0

    try:
        matrix = validate_matrix(args.bundles)
    except (MatrixValidationError, OSError, ValueError) as exc:
        print(f"RAFAELIA_ZERO_DEVICE_MATRIX=FAIL: {exc}", file=sys.stderr)
        return 1

    encoded = json.dumps(matrix, sort_keys=True, indent=2, ensure_ascii=False) + "\n"
    if args.output is not None:
        write_atomic(args.output, encoded)
    print(encoded, end="")
    print(f"RAFAELIA_ZERO_DEVICE_MATRIX={matrix['state']}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
