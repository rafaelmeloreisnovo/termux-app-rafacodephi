#!/usr/bin/env python3
"""Validate a complete RAFAELIA ZERO physical-device evidence bundle."""

from __future__ import annotations

import argparse
import copy
import hashlib
import importlib.util
import json
import pathlib
import sys
import tempfile
from typing import Any

ROOT = pathlib.Path(__file__).resolve().parents[1]
RECEIPT_VALIDATOR = ROOT / "scripts/validate_rafaelia_zero_device_receipt.py"
BUNDLE_BUILDER = ROOT / "scripts/create_rafaelia_zero_device_bundle.py"
BUNDLE_SCHEMA = "rafaelia.zero.device.evidence-bundle.v1"
CAPTURE_SCHEMA = "rafaelia.zero.device.capture.v1"
REQUIRED_FILES = {
    "receipt.json",
    "capture.json",
    "transcript.txt",
    "apk.bin",
    "manifest.json",
    "SHA256SUMS",
}
PAYLOAD_FILES = {"receipt.json", "capture.json", "transcript.txt", "apk.bin"}
ROLE_BY_ARCH = {1: "arm32-legacy", 2: "arm64-modern", 3: "x86_64", 4: "x86"}


class BundleValidationError(ValueError):
    pass


def require(condition: bool, message: str) -> None:
    if not condition:
        raise BundleValidationError(message)


def load_module(path: pathlib.Path, name: str):
    spec = importlib.util.spec_from_file_location(name, path)
    require(spec is not None and spec.loader is not None, f"cannot load {path.name}")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def sha256_file(path: pathlib.Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def read_object(path: pathlib.Path, label: str) -> dict[str, Any]:
    try:
        value = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        raise BundleValidationError(f"cannot read {label}: {exc}") from exc
    require(isinstance(value, dict), f"{label} must be an object")
    return value


def parse_sha256sums(path: pathlib.Path) -> dict[str, str]:
    entries: dict[str, str] = {}
    try:
        lines = path.read_text(encoding="utf-8").splitlines()
    except OSError as exc:
        raise BundleValidationError(f"cannot read SHA256SUMS: {exc}") from exc
    for line_number, line in enumerate(lines, 1):
        require(line, f"SHA256SUMS line {line_number} must not be empty")
        parts = line.split("  ", 1)
        require(len(parts) == 2, f"SHA256SUMS line {line_number} has invalid format")
        digest, name = parts
        require(len(digest) == 64 and all(ch in "0123456789abcdef" for ch in digest),
                f"SHA256SUMS line {line_number} has invalid digest")
        require(name in REQUIRED_FILES - {"SHA256SUMS"}, f"SHA256SUMS unexpected path: {name}")
        require(name not in entries, f"SHA256SUMS duplicate path: {name}")
        entries[name] = digest
    require(set(entries) == REQUIRED_FILES - {"SHA256SUMS"}, "SHA256SUMS file set mismatch")
    return entries


def validate_bundle(bundle_dir: pathlib.Path) -> dict[str, Any]:
    require(bundle_dir.exists(), f"bundle does not exist: {bundle_dir}")
    require(bundle_dir.is_dir(), f"bundle must be a directory: {bundle_dir}")
    require(not bundle_dir.is_symlink(), "bundle directory symlink is forbidden")

    names = {entry.name for entry in bundle_dir.iterdir()}
    require(names == REQUIRED_FILES, f"bundle file set mismatch: {sorted(names)}")
    for name in sorted(REQUIRED_FILES):
        path = bundle_dir / name
        require(path.is_file(), f"bundle entry must be a regular file: {name}")
        require(not path.is_symlink(), f"bundle symlink is forbidden: {name}")
        require(path.stat().st_size > 0, f"bundle file must not be empty: {name}")

    manifest = read_object(bundle_dir / "manifest.json", "manifest")
    require(manifest.get("schema") == BUNDLE_SCHEMA, f"manifest.schema must be {BUNDLE_SCHEMA}")
    require(manifest.get("result") == "PASS", "manifest.result must be PASS")
    require(manifest.get("claim_allowed_device_single") is True,
            "manifest.claim_allowed_device_single must be true")
    require(manifest.get("claim_allowed_device_matrix") is False,
            "a single bundle must not promote the device matrix")
    require(manifest.get("release_claim_allowed") is False,
            "debug evidence must not promote a release claim")

    files = manifest.get("files")
    require(isinstance(files, dict), "manifest.files must be an object")
    require(set(files) == PAYLOAD_FILES, "manifest.files set mismatch")
    for name in sorted(PAYLOAD_FILES):
        entry = files.get(name)
        require(isinstance(entry, dict), f"manifest.files.{name} must be an object")
        actual_sha = sha256_file(bundle_dir / name)
        actual_bytes = (bundle_dir / name).stat().st_size
        require(entry.get("sha256") == actual_sha, f"manifest digest mismatch: {name}")
        require(entry.get("bytes") == actual_bytes, f"manifest size mismatch: {name}")

    sums = parse_sha256sums(bundle_dir / "SHA256SUMS")
    for name, recorded in sums.items():
        require(recorded == sha256_file(bundle_dir / name), f"SHA256SUMS mismatch: {name}")

    receipt_path = bundle_dir / "receipt.json"
    apk_path = bundle_dir / "apk.bin"
    receipt_sha256 = sha256_file(receipt_path)
    apk_sha256 = sha256_file(apk_path)
    receipt = read_object(receipt_path, "receipt")
    receipt_validator = load_module(RECEIPT_VALIDATOR, "rafz_receipt_bundle")
    receipt_summary = receipt_validator.validate(receipt)

    capture = read_object(bundle_dir / "capture.json", "capture")
    require(capture.get("schema") == CAPTURE_SCHEMA, f"capture.schema must be {CAPTURE_SCHEMA}")
    require(capture.get("package") == receipt.get("package"), "capture package does not match receipt")
    receipt_device = receipt.get("device")
    require(isinstance(receipt_device, dict), "receipt.device must be an object")
    require(capture.get("device_fingerprint") == receipt_device.get("fingerprint"),
            "capture fingerprint does not match receipt")
    require(isinstance(capture.get("device_serial"), str) and capture["device_serial"],
            "capture.device_serial must be non-empty")
    require(isinstance(capture.get("installed_apk_path"), str) and capture["installed_apk_path"],
            "capture.installed_apk_path must be non-empty")

    expected_role = ROLE_BY_ARCH.get(receipt_summary["architecture_id"])
    require(expected_role is not None, "unsupported receipt architecture")
    require(manifest.get("role") == expected_role, "manifest role does not match receipt architecture")
    require(manifest.get("package") == capture.get("package"), "manifest package mismatch")

    device = manifest.get("device")
    require(isinstance(device, dict), "manifest.device must be an object")
    require(device.get("serial") == capture.get("device_serial"), "manifest serial mismatch")
    require(device.get("fingerprint") == capture.get("device_fingerprint"), "manifest fingerprint mismatch")
    require(device.get("installed_apk_path") == capture.get("installed_apk_path"),
            "manifest installed APK path mismatch")

    runtime = manifest.get("runtime")
    require(runtime == receipt_summary, "manifest runtime summary mismatch")
    limits = manifest.get("limits")
    require(isinstance(limits, dict), "manifest.limits must be an object")
    require(limits.get("debug_apk_only") is True, "debug_apk_only must be true")
    require(limits.get("single_bundle_does_not_promote_matrix") is True,
            "single-bundle matrix limit missing")
    require(limits.get("independent_reproduction") == "TOKEN_VAZIO",
            "independent reproduction must remain TOKEN_VAZIO")

    transcript = (bundle_dir / "transcript.txt").read_text(encoding="utf-8")
    require("RAFAELIA_ZERO_DEVICE_PROBE=PASS" in transcript, "transcript lacks PASS marker")
    require(f"receipt_sha256={receipt_sha256}" in transcript,
            "transcript is not bound to receipt SHA-256")
    require(f"apk_sha256={apk_sha256}" in transcript,
            "transcript is not bound to APK SHA-256")

    return {
        "schema": BUNDLE_SCHEMA,
        "result": "PASS",
        "role": expected_role,
        "package": manifest["package"],
        "device_serial": device["serial"],
        "device_fingerprint": device["fingerprint"],
        "architecture_id": receipt_summary["architecture_id"],
        "process_arch": receipt_summary["process_arch"],
        "receipt_sha256": receipt_sha256,
        "apk_sha256": apk_sha256,
        "manifest_sha256": sha256_file(bundle_dir / "manifest.json"),
        "claim_allowed_device_single": True,
        "claim_allowed_device_matrix": False,
    }


def self_test() -> None:
    builder = load_module(BUNDLE_BUILDER, "rafz_bundle_builder_for_validator")
    receipt_validator = load_module(RECEIPT_VALIDATOR, "rafz_receipt_for_bundle_validator")
    receipt = receipt_validator.sample_receipt()
    capture = {
        "schema": CAPTURE_SCHEMA,
        "package": receipt["package"],
        "device_serial": "SELFTEST-ARM32",
        "device_fingerprint": receipt["device"]["fingerprint"],
        "installed_apk_path": "/data/app/selftest/base.apk",
        "captured_at_unix_ms": receipt["timestamp_unix_ms"],
    }
    with tempfile.TemporaryDirectory(prefix="rafz-bundle-validator-") as temporary:
        root = pathlib.Path(temporary)
        receipt_path = root / "receipt-source.json"
        capture_path = root / "capture-source.json"
        transcript_path = root / "transcript-source.txt"
        apk_path = root / "probe.apk"
        bundle = root / "bundle"
        receipt_path.write_text(builder.canonical_json(receipt), encoding="utf-8")
        capture_path.write_text(builder.canonical_json(capture), encoding="utf-8")
        apk_path.write_bytes(b"synthetic-debug-apk")
        transcript_path.write_text(
            "RAFAELIA_ZERO_DEVICE_PROBE=PASS\n"
            f"receipt_sha256={builder.sha256_file(receipt_path)}\n"
            f"apk_sha256={builder.sha256_file(apk_path)}\n",
            encoding="utf-8",
        )
        builder.build_bundle(receipt_path, capture_path, apk_path, transcript_path, bundle)
        summary = validate_bundle(bundle)
        require(summary["result"] == "PASS", "valid self-test bundle rejected")

        original_apk = (bundle / "apk.bin").read_bytes()
        (bundle / "apk.bin").write_bytes(original_apk + b"tamper")
        try:
            validate_bundle(bundle)
        except BundleValidationError:
            pass
        else:
            raise AssertionError("tampered APK was accepted")
        (bundle / "apk.bin").write_bytes(original_apk)

        original_manifest = read_object(bundle / "manifest.json", "manifest")
        mutated = copy.deepcopy(original_manifest)
        mutated["claim_allowed_device_matrix"] = True
        (bundle / "manifest.json").write_text(builder.canonical_json(mutated), encoding="utf-8")
        try:
            validate_bundle(bundle)
        except BundleValidationError:
            pass
        else:
            raise AssertionError("single bundle promoted matrix claim")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("bundle", nargs="?", type=pathlib.Path)
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args()

    if args.self_test:
        try:
            self_test()
        except (AssertionError, BundleValidationError, OSError, ValueError) as exc:
            print(f"RAFAELIA_ZERO_DEVICE_BUNDLE_VALIDATOR_SELF_TEST=FAIL: {exc}", file=sys.stderr)
            return 1
        print("RAFAELIA_ZERO_DEVICE_BUNDLE_VALIDATOR_SELF_TEST=PASS")
        return 0

    if args.bundle is None:
        parser.error("bundle path is required unless --self-test is used")
    try:
        summary = validate_bundle(args.bundle)
    except (BundleValidationError, OSError, ValueError) as exc:
        print(f"RAFAELIA_ZERO_DEVICE_BUNDLE_VALIDATION=FAIL: {exc}", file=sys.stderr)
        return 1
    print(json.dumps(summary, sort_keys=True, separators=(",", ":")))
    print("RAFAELIA_ZERO_DEVICE_BUNDLE_VALIDATION=PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
