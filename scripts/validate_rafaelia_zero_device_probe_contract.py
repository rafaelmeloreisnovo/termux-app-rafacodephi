#!/usr/bin/env python3
"""Static contract gate for the RAFAELIA ZERO operational device proof path."""

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
BUNDLE_BUILDER = ROOT / "scripts/create_rafaelia_zero_device_bundle.py"
BUNDLE_VALIDATOR = ROOT / "scripts/validate_rafaelia_zero_device_bundle.py"
MATRIX_VALIDATOR = ROOT / "scripts/validate_rafaelia_zero_device_matrix.py"
MATRIX_REBUILDER = ROOT / "scripts/rebuild_rafaelia_zero_device_matrix.py"
PROBE_CONTRACT = ROOT / "configs/rafaelia-zero-device-probe-contract.json"
EVIDENCE_CONTRACT = ROOT / "configs/rafaelia-zero-operational-evidence-contract.json"


def require(condition: bool, message: str) -> None:
    if not condition:
        raise AssertionError(message)


def load_module(path: pathlib.Path, name: str):
    spec = importlib.util.spec_from_file_location(name, path)
    require(spec is not None and spec.loader is not None, f"cannot load {path.name}")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def main() -> int:
    required_paths = (
        DEBUG_MANIFEST,
        MAIN_MANIFEST,
        RELEASE_MANIFEST,
        ACTIVITY,
        RUNNER,
        RECEIPT_VALIDATOR,
        BUNDLE_BUILDER,
        BUNDLE_VALIDATOR,
        MATRIX_VALIDATOR,
        MATRIX_REBUILDER,
        PROBE_CONTRACT,
        EVIDENCE_CONTRACT,
    )
    for path in required_paths:
        require(path.is_file(), f"missing required file: {path.relative_to(ROOT)}")

    debug_manifest = DEBUG_MANIFEST.read_text(encoding="utf-8")
    main_manifest = MAIN_MANIFEST.read_text(encoding="utf-8")
    release_manifest = RELEASE_MANIFEST.read_text(encoding="utf-8")
    activity = ACTIVITY.read_text(encoding="utf-8")
    runner = RUNNER.read_text(encoding="utf-8")
    builder_text = BUNDLE_BUILDER.read_text(encoding="utf-8")
    matrix_text = MATRIX_VALIDATOR.read_text(encoding="utf-8")
    rebuilder_text = MATRIX_REBUILDER.read_text(encoding="utf-8")
    probe_contract = json.loads(PROBE_CONTRACT.read_text(encoding="utf-8"))
    evidence_contract = json.loads(EVIDENCE_CONTRACT.read_text(encoding="utf-8"))

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

    runner_requirements = (
        "run-as",
        "validate_rafaelia_zero_device_receipt.py",
        "create_rafaelia_zero_device_bundle.py",
        "validate_rafaelia_zero_device_bundle.py",
        "rebuild_rafaelia_zero_device_matrix.py",
        "capture.json",
        "transcript.txt",
        "apk.bin",
        "receipt_sha256=",
        "apk_sha256=",
        "input_installed_apk_hash_match=PASS",
        "expected exactly one installed APK path",
        "RAFAELIA_ZERO_OPERATIONAL_EVIDENCE=PASS",
    )
    for token in runner_requirements:
        require(token in runner, f"runner contract token missing: {token}")
    require("RUN_RECORDED_STATUS" in runner and "return \"$RUN_RECORDED_STATUS\"" in runner,
            "runner must preserve command failures through transcript capture")

    require("os.replace(staging, output_dir)" in builder_text,
            "bundle builder must publish with atomic directory rename")
    require("fsync_directory(staging)" in builder_text,
            "bundle builder must fsync staging before publication")
    require("receipt_sha256=" in builder_text and "apk_sha256=" in builder_text,
            "bundle builder must bind transcript to receipt and APK hashes")
    require("os.replace(temporary, path)" in matrix_text,
            "matrix validator must publish output atomically")
    require("one-validated-bundle-per-role" in rebuilder_text,
            "matrix rebuilder selection policy missing")
    require("direct child of evidence root" in rebuilder_text,
            "matrix rebuilder path boundary missing")
    require("selection pointer symlink is forbidden" in rebuilder_text,
            "matrix rebuilder symlink rejection missing")

    require(probe_contract.get("schema") == "rafaelia.zero.device-probe-contract.v1",
            "unexpected probe contract schema")
    component_policy = probe_contract.get("component", {})
    promotion = probe_contract.get("promotion", {})
    states = probe_contract.get("states", {})
    require(component_policy.get("source_set") == "debug", "probe contract must pin debug source set")
    require(component_policy.get("activity") == component, "probe contract activity mismatch")
    require(component_policy.get("required_permission") == "android.permission.DUMP",
            "probe contract permission mismatch")
    require(component_policy.get("release_present") is False,
            "probe contract must prohibit release component")
    require(promotion.get("static_contract_pass_promotes_device_claim") is False,
            "static PASS must not promote device claim")
    require(promotion.get("valid_device_receipt_required") is True,
            "valid device receipt must be required")
    require(promotion.get("apk_sha256_required") is True,
            "APK SHA-256 must be required")
    require(states.get("physical_device_receipt") == "TOKEN_VAZIO",
            "physical receipt must remain TOKEN_VAZIO in source contract")

    require(evidence_contract.get("schema") == "rafaelia.zero.operational-evidence-contract.v1",
            "unexpected operational evidence contract schema")
    authority = evidence_contract.get("authority", {})
    bundle = evidence_contract.get("bundle", {})
    publication = evidence_contract.get("publication", {})
    selection = evidence_contract.get("selection", {})
    capture = evidence_contract.get("capture", {})
    targets = evidence_contract.get("required_targets", {})
    matrix_promotion = evidence_contract.get("promotion", {})
    current_state = evidence_contract.get("current_state", {})
    require(authority.get("matrix_rebuilder") == "scripts/rebuild_rafaelia_zero_device_matrix.py",
            "matrix rebuilder authority mismatch")
    require(bundle.get("schema") == "rafaelia.zero.device.evidence-bundle.v1",
            "evidence bundle schema mismatch")
    require(bundle.get("digest") == "sha256", "bundle digest must be sha256")
    require(bundle.get("symlink_policy") == "forbidden", "bundle symlinks must be forbidden")
    require(set(bundle.get("transcript_bindings", [])) == {"receipt_sha256", "apk_sha256"},
            "transcript bindings must cover receipt and APK")
    require(publication.get("bundle") == "atomic-staging-fsync-rename",
            "bundle publication contract mismatch")
    require(publication.get("matrix") == "atomic-temp-fsync-replace",
            "matrix publication contract mismatch")
    require(publication.get("selection_pointer") == "atomic-temp-fsync-replace",
            "selection pointer publication mismatch")
    require(selection.get("policy") == "one-validated-bundle-per-role",
            "selection policy mismatch")
    require(selection.get("pointer_payload") == "direct-child-bundle-name-only",
            "selection pointer payload mismatch")
    require(selection.get("history_preserved") is True,
            "selection policy must preserve historical bundles")
    require(selection.get("matrix_reads_selected_only") is True,
            "matrix must read selected bundles only")
    require(capture.get("schema") == "rafaelia.zero.device.capture.v1",
            "capture schema mismatch")
    require(capture.get("installed_apk_source") == "adb-pull-of-pm-path",
            "installed APK capture source mismatch")
    require(capture.get("single_installed_apk_path_required") is True,
            "single installed APK path must be required")
    require(capture.get("input_hash_must_match_installed_when_provided") is True,
            "input/installed APK hash match must be required")
    require(set(targets) == {"arm32-legacy", "arm64-modern"},
            "required device targets must be ARM32 and ARM64")
    require(targets["arm32-legacy"].get("architecture_id") == 1,
            "ARM32 target architecture id mismatch")
    require(targets["arm64-modern"].get("architecture_id") == 2,
            "ARM64 target architecture id mismatch")
    require(matrix_promotion.get("static_contract_can_promote") is False,
            "static contract must not promote matrix")
    require(matrix_promotion.get("single_bundle_can_promote_matrix") is False,
            "single bundle must not promote matrix")
    require(matrix_promotion.get("release_claim_from_debug_evidence") is False,
            "debug evidence must not promote release")
    require(current_state.get("arm32-legacy") == "TOKEN_VAZIO",
            "ARM32 source state must remain TOKEN_VAZIO")
    require(current_state.get("arm64-modern") == "TOKEN_VAZIO",
            "ARM64 source state must remain TOKEN_VAZIO")
    require(current_state.get("claim_allowed_device_matrix") is False,
            "source contract must not pre-authorize device matrix")

    receipt_module = load_module(RECEIPT_VALIDATOR, "rafz_receipt_contract")
    builder_module = load_module(BUNDLE_BUILDER, "rafz_builder_contract")
    bundle_module = load_module(BUNDLE_VALIDATOR, "rafz_bundle_contract")
    matrix_module = load_module(MATRIX_VALIDATOR, "rafz_matrix_contract")
    rebuilder_module = load_module(MATRIX_REBUILDER, "rafz_rebuilder_contract")
    receipt_module.self_test()
    builder_module.self_test()
    bundle_module.self_test()
    matrix_module.self_test()
    rebuilder_module.self_test()

    print("RAFAELIA_ZERO_DEVICE_PROBE_CONTRACT=PASS")
    print("release_component_present=false")
    print("debug_permission=android.permission.DUMP")
    print("receipt_validator_self_test=PASS")
    print("bundle_builder_self_test=PASS")
    print("bundle_validator_self_test=PASS")
    print("matrix_validator_self_test=PASS")
    print("matrix_rebuilder_self_test=PASS")
    print("installed_apk_capture=adb-pull-of-pm-path")
    print("bundle_publication=atomic")
    print("matrix_selection=one-validated-bundle-per-role")
    print("arm32_device_receipt=TOKEN_VAZIO")
    print("arm64_device_receipt=TOKEN_VAZIO")
    print("claim_allowed_device_matrix=false")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (AssertionError, json.JSONDecodeError, OSError, ValueError) as exc:
        print(f"RAFAELIA_ZERO_DEVICE_PROBE_CONTRACT=FAIL: {exc}", file=sys.stderr)
        raise SystemExit(1)
