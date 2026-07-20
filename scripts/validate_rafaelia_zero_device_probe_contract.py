#!/usr/bin/env python3
"""Static contract gate for the RAFAELIA ZERO debug device probe."""

from __future__ import annotations

import importlib.util
import json
import pathlib
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]
DEBUG_MANIFEST = ROOT / "app/src/debug/AndroidManifest.xml"
MAIN_MANIFEST = ROOT / "app/src/main/AndroidManifest.xml"
RELEASE_MANIFEST = ROOT / "app/src/release/AndroidManifest.xml"
ACTIVITY = ROOT / "app/src/debug/java/com/termux/app/rafaelia/RafaeliaZeroProbeActivity.java"
RUNNER = ROOT / "scripts/run_rafaelia_zero_device_probe.sh"
RECEIPT_VALIDATOR = ROOT / "scripts/validate_rafaelia_zero_device_receipt.py"
CONTRACT = ROOT / "configs/rafaelia-zero-device-probe-contract.json"


def require(condition: bool, message: str) -> None:
    if not condition:
        raise AssertionError(message)


def load_receipt_validator():
    spec = importlib.util.spec_from_file_location("rafz_receipt", RECEIPT_VALIDATOR)
    require(spec is not None and spec.loader is not None, "cannot load receipt validator")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def main() -> int:
    for path in (
        DEBUG_MANIFEST,
        MAIN_MANIFEST,
        RELEASE_MANIFEST,
        ACTIVITY,
        RUNNER,
        RECEIPT_VALIDATOR,
        CONTRACT,
    ):
        require(path.is_file(), f"missing required file: {path.relative_to(ROOT)}")

    debug_manifest = DEBUG_MANIFEST.read_text(encoding="utf-8")
    main_manifest = MAIN_MANIFEST.read_text(encoding="utf-8")
    release_manifest = RELEASE_MANIFEST.read_text(encoding="utf-8")
    activity = ACTIVITY.read_text(encoding="utf-8")
    runner = RUNNER.read_text(encoding="utf-8")
    contract = json.loads(CONTRACT.read_text(encoding="utf-8"))

    component = "com.termux.app.rafaelia.RafaeliaZeroProbeActivity"
    require(component in debug_manifest, "probe activity missing from debug manifest")
    require(component not in main_manifest, "probe activity must not ship in main manifest")
    require(component not in release_manifest, "probe activity must not ship in release manifest")
    require('android:permission="android.permission.DUMP"' in debug_manifest,
            "probe must require android.permission.DUMP")
    require('android:exported="true"' in debug_manifest,
            "adb probe activity must be explicitly exported in debug source set")
    require("RafaeliaZeroRuntime.ingestDirect" in activity, "native ingest is not exercised")
    require("acceptedAfter == acceptedBefore + 1" in activity, "accepted-count invariant missing")
    require("digestAfter != digestBefore" in activity, "digest-change invariant missing")
    require("latest.json.tmp" in activity and "renameTo(target)" in activity,
            "receipt must be written atomically")
    require("claim_allowed_device" in activity, "device claim gate missing")
    require("run-as" in runner, "runner must capture from app-private storage with run-as")
    require("validate_rafaelia_zero_device_receipt.py" in runner,
            "runner must invoke receipt validator")
    require("sha256" in runner.lower(), "runner must hash the captured receipt")

    require(contract.get("schema") == "rafaelia.zero.device-probe-contract.v1",
            "unexpected machine-readable contract schema")
    component_policy = contract.get("component", {})
    promotion = contract.get("promotion", {})
    states = contract.get("states", {})
    require(component_policy.get("source_set") == "debug", "contract must pin debug source set")
    require(component_policy.get("activity") == component, "contract activity mismatch")
    require(component_policy.get("required_permission") == "android.permission.DUMP",
            "contract permission mismatch")
    require(component_policy.get("release_present") is False,
            "contract must prohibit release component")
    require(promotion.get("static_contract_pass_promotes_device_claim") is False,
            "static PASS must not promote device claim")
    require(promotion.get("valid_device_receipt_required") is True,
            "valid device receipt must be required")
    require(promotion.get("apk_sha256_required") is True,
            "APK SHA-256 must be required")
    require(states.get("physical_device_receipt") == "TOKEN_VAZIO",
            "physical receipt must remain TOKEN_VAZIO in source contract")

    module = load_receipt_validator()
    module.self_test()

    print("RAFAELIA_ZERO_DEVICE_PROBE_CONTRACT=PASS")
    print("release_component_present=false")
    print("debug_permission=android.permission.DUMP")
    print("receipt_validator_self_test=PASS")
    print("physical_device_receipt=TOKEN_VAZIO")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (AssertionError, json.JSONDecodeError) as exc:
        print(f"RAFAELIA_ZERO_DEVICE_PROBE_CONTRACT=FAIL: {exc}", file=sys.stderr)
        raise SystemExit(1)
