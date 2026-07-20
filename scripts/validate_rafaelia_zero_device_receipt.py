#!/usr/bin/env python3
"""Validate RAFAELIA ZERO physical-device probe receipts.

This validator proves only that a receipt is internally coherent and reports a
successful native run. It does not independently prove the device identity or
that the receipt was produced by an unmodified APK; those require APK/hash and
capture-chain evidence supplied by the caller.
"""

from __future__ import annotations

import argparse
import copy
import json
import pathlib
import sys
from typing import Any

SCHEMA = "rafaelia.zero.device.probe.v1"
REQUIRED_CHECKS = (
    "debuggable",
    "library_available",
    "init_ok",
    "architecture_match",
    "max_payload_match",
    "ingest_ok",
    "accepted_increment",
    "rejected_stable",
    "digest_changed",
    "digest_nonzero",
    "null_guard",
    "range_guard",
    "page_size_valid",
)
ARCH_BY_PROCESS = {
    "aarch64": 2,
    "arm64": 2,
    "armv7l": 1,
    "armv7": 1,
    "arm": 1,
    "x86_64": 3,
    "amd64": 3,
    "i686": 4,
    "i386": 4,
    "x86": 4,
}


class ReceiptError(ValueError):
    pass


def require(condition: bool, message: str) -> None:
    if not condition:
        raise ReceiptError(message)


def as_dict(value: Any, name: str) -> dict[str, Any]:
    require(isinstance(value, dict), f"{name} must be an object")
    return value


def as_int(value: Any, name: str) -> int:
    require(isinstance(value, int) and not isinstance(value, bool), f"{name} must be an integer")
    return value


def expected_architecture(process_arch: str) -> int:
    normalized = process_arch.strip().lower()
    for token, arch_id in ARCH_BY_PROCESS.items():
        if normalized == token or token in normalized:
            return arch_id
    return 0


def validate(receipt: dict[str, Any]) -> dict[str, Any]:
    require(receipt.get("schema") == SCHEMA, f"schema must be {SCHEMA}")
    require(receipt.get("result") == "PASS", "result must be PASS")
    require(receipt.get("claim_allowed_device") is True, "claim_allowed_device must be true")
    require(receipt.get("debuggable") is True, "debuggable must be true")
    require(isinstance(receipt.get("package"), str) and receipt["package"], "package must be non-empty")

    device = as_dict(receipt.get("device"), "device")
    native = as_dict(receipt.get("native"), "native")
    observed = as_dict(receipt.get("observed"), "observed")
    checks = as_dict(receipt.get("checks"), "checks")

    for name in REQUIRED_CHECKS:
        require(checks.get(name) is True, f"checks.{name} must be true")

    process_arch = device.get("process_arch")
    require(isinstance(process_arch, str) and process_arch, "device.process_arch must be non-empty")
    page_size = as_int(device.get("page_size"), "device.page_size")
    require(page_size > 0 and page_size & (page_size - 1) == 0, "device.page_size must be a power of two")

    init_status = as_int(native.get("init_status"), "native.init_status")
    ingest_status = as_int(native.get("ingest_status"), "native.ingest_status")
    null_guard_status = as_int(native.get("null_guard_status"), "native.null_guard_status")
    range_guard_status = as_int(native.get("range_guard_status"), "native.range_guard_status")
    architecture_id = as_int(native.get("architecture_id"), "native.architecture_id")
    expected_id = as_int(native.get("expected_architecture_id"), "native.expected_architecture_id")
    max_payload = as_int(native.get("max_payload"), "native.max_payload")

    require(native.get("available") is True, "native.available must be true")
    require(init_status == 0, "native.init_status must be 0")
    require(ingest_status == 0, "native.ingest_status must be 0")
    require(null_guard_status == -1, "native.null_guard_status must be -1")
    require(range_guard_status == -6, "native.range_guard_status must be -6")
    require(architecture_id in {1, 2, 3, 4}, "native.architecture_id must be an Android ABI id")
    require(expected_id == expected_architecture(process_arch), "expected architecture does not match process_arch")
    require(architecture_id == expected_id, "native architecture does not match process architecture")
    require(max_payload == 1024, "native.max_payload must be 1024")

    payload_bytes = as_int(observed.get("payload_bytes"), "observed.payload_bytes")
    accepted_before = as_int(observed.get("accepted_before"), "observed.accepted_before")
    accepted_after = as_int(observed.get("accepted_after"), "observed.accepted_after")
    rejected_before = as_int(observed.get("rejected_before"), "observed.rejected_before")
    rejected_after = as_int(observed.get("rejected_after"), "observed.rejected_after")
    digest_before = as_int(observed.get("digest_before"), "observed.digest_before")
    digest_after = as_int(observed.get("digest_after"), "observed.digest_after")

    require(payload_bytes == 20, "observed.payload_bytes must be 20")
    require(accepted_after == accepted_before + 1, "accepted count must increment exactly once")
    require(rejected_after == rejected_before, "rejected count must remain stable")
    require(digest_after != digest_before, "state digest must change")
    require(digest_after != 0, "state digest must be non-zero")

    timestamp = as_int(receipt.get("timestamp_unix_ms"), "timestamp_unix_ms")
    require(timestamp > 0, "timestamp_unix_ms must be positive")

    return {
        "schema": SCHEMA,
        "result": "PASS",
        "package": receipt["package"],
        "process_arch": process_arch,
        "architecture_id": architecture_id,
        "page_size": page_size,
        "accepted_delta": accepted_after - accepted_before,
        "digest_after": digest_after,
        "claim_allowed_device": True,
    }


def sample_receipt() -> dict[str, Any]:
    return {
        "schema": SCHEMA,
        "result": "PASS",
        "claim_allowed_device": True,
        "timestamp_unix_ms": 1784548800000,
        "package": "com.termux.rafacodephi",
        "debuggable": True,
        "device": {
            "manufacturer": "test",
            "model": "test",
            "device": "test",
            "fingerprint": "test/fingerprint",
            "sdk_int": 35,
            "process_arch": "armv7l",
            "supported_abis": "armeabi-v7a",
            "page_size": 4096,
        },
        "native": {
            "init_status": 0,
            "available": True,
            "architecture_id": 1,
            "expected_architecture_id": 1,
            "max_payload": 1024,
            "ingest_status": 0,
            "null_guard_status": -1,
            "range_guard_status": -6,
        },
        "observed": {
            "payload_bytes": 20,
            "source": 5928523890307387202,
            "sequence": 42,
            "accepted_before": 0,
            "accepted_after": 1,
            "rejected_before": 0,
            "rejected_after": 0,
            "digest_before": 1,
            "digest_after": 2,
        },
        "checks": {name: True for name in REQUIRED_CHECKS},
    }


def self_test() -> None:
    validate(sample_receipt())
    mutations = (
        ("result", lambda r: r.__setitem__("result", "FAIL")),
        ("claim", lambda r: r.__setitem__("claim_allowed_device", False)),
        ("arch", lambda r: r["native"].__setitem__("architecture_id", 2)),
        ("accepted", lambda r: r["observed"].__setitem__("accepted_after", 7)),
        ("digest", lambda r: r["observed"].__setitem__("digest_after", 1)),
        ("guard", lambda r: r["checks"].__setitem__("range_guard", False)),
        ("page", lambda r: r["device"].__setitem__("page_size", 6000)),
    )
    for name, mutate in mutations:
        candidate = copy.deepcopy(sample_receipt())
        mutate(candidate)
        try:
            validate(candidate)
        except ReceiptError:
            continue
        raise AssertionError(f"self-test mutation was accepted: {name}")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("receipt", nargs="?", type=pathlib.Path)
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args()

    if args.self_test:
        self_test()
        print("RAFAELIA_ZERO_DEVICE_RECEIPT_SELF_TEST=PASS")
        return 0

    if args.receipt is None:
        parser.error("receipt path is required unless --self-test is used")

    try:
        data = json.loads(args.receipt.read_text(encoding="utf-8"))
        summary = validate(as_dict(data, "receipt"))
    except (OSError, json.JSONDecodeError, ReceiptError) as exc:
        print(f"RAFAELIA_ZERO_DEVICE_RECEIPT=FAIL: {exc}", file=sys.stderr)
        return 1

    print(json.dumps(summary, sort_keys=True, separators=(",", ":")))
    print("RAFAELIA_ZERO_DEVICE_RECEIPT=PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
