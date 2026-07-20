#!/usr/bin/env python3
"""Create a deterministic RAFAELIA ZERO physical-device evidence bundle.

The builder validates the native receipt before copying any evidence. It then
normalizes filenames, computes SHA-256 digests and emits a fail-closed manifest.
A single valid bundle proves only one measured debug-device execution; it does
not promote the required ARM32+ARM64 matrix by itself.
"""

from __future__ import annotations

import argparse
import hashlib
import importlib.util
import json
import os
import pathlib
import shutil
import sys
import tempfile
from typing import Any

ROOT = pathlib.Path(__file__).resolve().parents[1]
RECEIPT_VALIDATOR = ROOT / "scripts/validate_rafaelia_zero_device_receipt.py"
BUNDLE_SCHEMA = "rafaelia.zero.device.evidence-bundle.v1"
CAPTURE_SCHEMA = "rafaelia.zero.device.capture.v1"
FIXED_FILES = {
    "receipt": "receipt.json",
    "capture": "capture.json",
    "transcript": "transcript.txt",
    "apk": "apk.bin",
}
ROLE_BY_ARCH = {
    1: "arm32-legacy",
    2: "arm64-modern",
    3: "x86_64",
    4: "x86",
}


class BundleBuildError(ValueError):
    pass


def require(condition: bool, message: str) -> None:
    if not condition:
        raise BundleBuildError(message)


def load_receipt_validator():
    spec = importlib.util.spec_from_file_location("rafz_receipt", RECEIPT_VALIDATOR)
    require(spec is not None and spec.loader is not None, "cannot load receipt validator")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def sha256_file(path: pathlib.Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def canonical_json(data: Any) -> str:
    return json.dumps(data, sort_keys=True, indent=2, ensure_ascii=False) + "\n"


def write_atomic(path: pathlib.Path, text: str) -> None:
    temporary = path.with_name(path.name + ".tmp")
    with temporary.open("w", encoding="utf-8", newline="\n") as stream:
        stream.write(text)
        stream.flush()
        os.fsync(stream.fileno())
    os.replace(temporary, path)


def fsync_directory(path: pathlib.Path) -> None:
    descriptor = os.open(path, os.O_RDONLY)
    try:
        os.fsync(descriptor)
    finally:
        os.close(descriptor)


def read_json_object(path: pathlib.Path, label: str) -> dict[str, Any]:
    try:
        data = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        raise BundleBuildError(f"cannot read {label}: {exc}") from exc
    require(isinstance(data, dict), f"{label} must be a JSON object")
    return data


def validate_capture(capture: dict[str, Any], receipt: dict[str, Any]) -> None:
    require(capture.get("schema") == CAPTURE_SCHEMA, f"capture.schema must be {CAPTURE_SCHEMA}")
    for field in (
        "package",
        "device_serial",
        "device_fingerprint",
        "installed_apk_path",
        "captured_at_unix_ms",
    ):
        require(field in capture, f"capture.{field} is required")
    require(isinstance(capture["package"], str) and capture["package"], "capture.package must be non-empty")
    require(isinstance(capture["device_serial"], str) and capture["device_serial"], "capture.device_serial must be non-empty")
    require(
        isinstance(capture["device_fingerprint"], str) and capture["device_fingerprint"],
        "capture.device_fingerprint must be non-empty",
    )
    require(
        isinstance(capture["installed_apk_path"], str) and capture["installed_apk_path"],
        "capture.installed_apk_path must be non-empty",
    )
    require(
        isinstance(capture["captured_at_unix_ms"], int)
        and not isinstance(capture["captured_at_unix_ms"], bool)
        and capture["captured_at_unix_ms"] > 0,
        "capture.captured_at_unix_ms must be a positive integer",
    )
    require(capture["package"] == receipt.get("package"), "capture.package does not match receipt.package")
    receipt_device = receipt.get("device")
    require(isinstance(receipt_device, dict), "receipt.device must be an object")
    require(
        capture["device_fingerprint"] == receipt_device.get("fingerprint"),
        "capture fingerprint does not match receipt fingerprint",
    )


def checked_input(path: pathlib.Path, label: str) -> pathlib.Path:
    require(path.exists(), f"{label} does not exist: {path}")
    require(path.is_file(), f"{label} must be a regular file: {path}")
    require(not path.is_symlink(), f"{label} symlink is forbidden: {path}")
    require(path.stat().st_size > 0, f"{label} must not be empty: {path}")
    return path


def build_bundle(
    receipt_path: pathlib.Path,
    capture_path: pathlib.Path,
    apk_path: pathlib.Path,
    transcript_path: pathlib.Path,
    output_dir: pathlib.Path,
) -> dict[str, Any]:
    receipt_path = checked_input(receipt_path, "receipt")
    capture_path = checked_input(capture_path, "capture")
    apk_path = checked_input(apk_path, "apk")
    transcript_path = checked_input(transcript_path, "transcript")

    receipt = read_json_object(receipt_path, "receipt")
    validator = load_receipt_validator()
    receipt_summary = validator.validate(receipt)
    capture = read_json_object(capture_path, "capture")
    validate_capture(capture, receipt)

    receipt_sha256 = sha256_file(receipt_path)
    apk_sha256 = sha256_file(apk_path)
    transcript = transcript_path.read_text(encoding="utf-8", errors="strict")
    require("RAFAELIA_ZERO_DEVICE_PROBE=PASS" in transcript, "transcript lacks probe PASS marker")
    require(f"receipt_sha256={receipt_sha256}" in transcript, "transcript is not bound to receipt SHA-256")
    require(f"apk_sha256={apk_sha256}" in transcript, "transcript is not bound to APK SHA-256")

    architecture_id = receipt_summary["architecture_id"]
    role = ROLE_BY_ARCH.get(architecture_id)
    require(role is not None, f"unsupported architecture_id: {architecture_id}")

    require(not output_dir.exists(), f"output path already exists: {output_dir}")
    output_dir.parent.mkdir(parents=True, exist_ok=True)
    staging = output_dir.parent / f".{output_dir.name}.tmp-{os.getpid()}"
    require(not staging.exists(), f"staging path already exists: {staging}")
    staging.mkdir(parents=False, exist_ok=False)

    sources = {
        FIXED_FILES["receipt"]: receipt_path,
        FIXED_FILES["capture"]: capture_path,
        FIXED_FILES["transcript"]: transcript_path,
        FIXED_FILES["apk"]: apk_path,
    }
    try:
        for destination_name, source in sources.items():
            shutil.copyfile(source, staging / destination_name)

        file_entries: dict[str, dict[str, Any]] = {}
        for name in sorted(sources):
            path = staging / name
            file_entries[name] = {
                "sha256": sha256_file(path),
                "bytes": path.stat().st_size,
            }

        manifest = {
            "schema": BUNDLE_SCHEMA,
            "result": "PASS",
            "role": role,
            "claim_allowed_device_single": True,
            "claim_allowed_device_matrix": False,
            "release_claim_allowed": False,
            "created_at_unix_ms": capture["captured_at_unix_ms"],
            "package": capture["package"],
            "device": {
                "serial": capture["device_serial"],
                "fingerprint": capture["device_fingerprint"],
                "installed_apk_path": capture["installed_apk_path"],
            },
            "runtime": receipt_summary,
            "files": file_entries,
            "limits": {
                "debug_apk_only": True,
                "single_bundle_does_not_promote_matrix": True,
                "independent_reproduction": "TOKEN_VAZIO",
            },
        }
        manifest_path = staging / "manifest.json"
        write_atomic(manifest_path, canonical_json(manifest))

        sums = []
        for name in sorted((*sources.keys(), "manifest.json")):
            sums.append(f"{sha256_file(staging / name)}  {name}")
        write_atomic(staging / "SHA256SUMS", "\n".join(sums) + "\n")
        fsync_directory(staging)
        os.replace(staging, output_dir)
        fsync_directory(output_dir.parent)
        return manifest
    except BaseException:
        if staging.exists():
            shutil.rmtree(staging, ignore_errors=True)
        raise


def self_test() -> None:
    validator = load_receipt_validator()
    receipt = validator.sample_receipt()
    capture = {
        "schema": CAPTURE_SCHEMA,
        "package": receipt["package"],
        "device_serial": "SELFTEST-ARM32",
        "device_fingerprint": receipt["device"]["fingerprint"],
        "installed_apk_path": "/data/app/selftest/base.apk",
        "captured_at_unix_ms": receipt["timestamp_unix_ms"],
    }
    with tempfile.TemporaryDirectory(prefix="rafz-bundle-builder-") as temporary:
        root = pathlib.Path(temporary)
        receipt_path = root / "receipt.json"
        capture_path = root / "capture.json"
        transcript_path = root / "transcript.txt"
        apk_path = root / "probe.apk"
        output = root / "bundle"
        receipt_path.write_text(canonical_json(receipt), encoding="utf-8")
        capture_path.write_text(canonical_json(capture), encoding="utf-8")
        apk_path.write_bytes(b"synthetic-debug-apk")
        transcript_path.write_text(
            "RAFAELIA_ZERO_DEVICE_PROBE=PASS\n"
            f"receipt_sha256={sha256_file(receipt_path)}\n"
            f"apk_sha256={sha256_file(apk_path)}\n",
            encoding="utf-8",
        )
        manifest = build_bundle(receipt_path, capture_path, apk_path, transcript_path, output)
        require(manifest["result"] == "PASS", "self-test manifest did not pass")
        require(manifest["role"] == "arm32-legacy", "self-test role mismatch")
        require((output / "SHA256SUMS").is_file(), "self-test SHA256SUMS missing")
        require(not any(root.glob(".bundle.tmp-*")), "self-test left a staging directory")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--receipt", type=pathlib.Path)
    parser.add_argument("--capture", type=pathlib.Path)
    parser.add_argument("--apk", type=pathlib.Path)
    parser.add_argument("--transcript", type=pathlib.Path)
    parser.add_argument("--output", type=pathlib.Path)
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args()

    if args.self_test:
        try:
            self_test()
        except (BundleBuildError, OSError, ValueError) as exc:
            print(f"RAFAELIA_ZERO_DEVICE_BUNDLE_BUILDER_SELF_TEST=FAIL: {exc}", file=sys.stderr)
            return 1
        print("RAFAELIA_ZERO_DEVICE_BUNDLE_BUILDER_SELF_TEST=PASS")
        return 0

    required = (args.receipt, args.capture, args.apk, args.transcript, args.output)
    if any(value is None for value in required):
        parser.error("--receipt, --capture, --apk, --transcript and --output are required")

    try:
        manifest = build_bundle(args.receipt, args.capture, args.apk, args.transcript, args.output)
    except (BundleBuildError, OSError, ValueError) as exc:
        print(f"RAFAELIA_ZERO_DEVICE_BUNDLE=FAIL: {exc}", file=sys.stderr)
        return 1

    print(json.dumps(manifest, sort_keys=True, separators=(",", ":")))
    print(f"RAFAELIA_ZERO_DEVICE_BUNDLE=PASS:{args.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
