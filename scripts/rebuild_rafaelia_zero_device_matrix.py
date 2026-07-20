#!/usr/bin/env python3
"""Select canonical per-role bundles and rebuild the RAFAELIA ZERO matrix."""

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
MATRIX_VALIDATOR = ROOT / "scripts/validate_rafaelia_zero_device_matrix.py"
BUNDLE_BUILDER = ROOT / "scripts/create_rafaelia_zero_device_bundle.py"
RECEIPT_VALIDATOR = ROOT / "scripts/validate_rafaelia_zero_device_receipt.py"
ROLES = ("arm32-legacy", "arm64-modern", "x86", "x86_64")


class SelectionError(ValueError):
    pass


def require(condition: bool, message: str) -> None:
    if not condition:
        raise SelectionError(message)


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


def direct_child(root: pathlib.Path, candidate: pathlib.Path, label: str) -> pathlib.Path:
    root_resolved = root.resolve(strict=True)
    candidate_resolved = candidate.resolve(strict=True)
    require(candidate_resolved.parent == root_resolved,
            f"{label} must be a direct child of evidence root")
    require(candidate.is_dir(), f"{label} must be a directory")
    require(not candidate.is_symlink(), f"{label} symlink is forbidden")
    return candidate


def select_bundle(evidence_root: pathlib.Path, role: str, bundle: pathlib.Path) -> None:
    require(role in ROLES, f"unsupported selection role: {role}")
    bundle_validator = load_module(BUNDLE_VALIDATOR, "rafz_selection_bundle")
    checked = direct_child(evidence_root, bundle, "selected bundle")
    summary = bundle_validator.validate_bundle(checked)
    require(summary["role"] == role,
            f"selected bundle role {summary['role']} does not match requested role {role}")
    write_atomic(evidence_root / f"selected-{role}.txt", checked.name + "\n")


def read_selection(evidence_root: pathlib.Path, role: str) -> pathlib.Path | None:
    pointer = evidence_root / f"selected-{role}.txt"
    if not pointer.exists():
        return None
    require(pointer.is_file(), f"selection pointer must be a regular file: {pointer.name}")
    require(not pointer.is_symlink(), f"selection pointer symlink is forbidden: {pointer.name}")
    lines = pointer.read_text(encoding="utf-8").splitlines()
    require(len(lines) == 1 and lines[0], f"selection pointer must contain exactly one bundle name: {pointer.name}")
    name = lines[0]
    require("/" not in name and "\\" not in name and name not in {".", ".."},
            f"selection pointer contains unsafe path: {pointer.name}")
    return direct_child(evidence_root, evidence_root / name, f"selection {role}")


def rebuild(
    evidence_root: pathlib.Path,
    select_role: str | None = None,
    select_path: pathlib.Path | None = None,
) -> dict[str, Any]:
    evidence_root.mkdir(parents=True, exist_ok=True)
    require(evidence_root.is_dir(), "evidence root must be a directory")
    require(not evidence_root.is_symlink(), "evidence root symlink is forbidden")
    require((select_role is None) == (select_path is None),
            "select_role and select_path must be supplied together")
    if select_role is not None and select_path is not None:
        select_bundle(evidence_root, select_role, select_path)

    selected: dict[str, pathlib.Path] = {}
    for role in ROLES:
        bundle = read_selection(evidence_root, role)
        if bundle is not None:
            selected[role] = bundle

    matrix_validator = load_module(MATRIX_VALIDATOR, "rafz_selection_matrix")
    matrix = matrix_validator.validate_matrix([selected[role] for role in ROLES if role in selected])
    matrix["selected_bundles"] = {role: selected[role].name for role in ROLES if role in selected}
    matrix["selection_policy"] = "one-validated-bundle-per-role"
    encoded = json.dumps(matrix, sort_keys=True, indent=2, ensure_ascii=False) + "\n"
    matrix_validator.write_atomic(evidence_root / "matrix.json", encoded)
    return matrix


def self_test() -> None:
    builder = load_module(BUNDLE_BUILDER, "rafz_selection_builder")
    receipt_validator = load_module(RECEIPT_VALIDATOR, "rafz_selection_receipt")
    matrix_helper = load_module(MATRIX_VALIDATOR, "rafz_selection_matrix_helper")

    arm32 = receipt_validator.sample_receipt()
    arm32["device"]["fingerprint"] = "selection/arm32/fingerprint"
    arm32["timestamp_unix_ms"] = 1784548800101
    arm64 = copy.deepcopy(arm32)
    arm64["device"]["fingerprint"] = "selection/arm64/fingerprint"
    arm64["device"]["process_arch"] = "aarch64"
    arm64["device"]["supported_abis"] = "arm64-v8a,armeabi-v7a"
    arm64["native"]["architecture_id"] = 2
    arm64["native"]["expected_architecture_id"] = 2
    arm64["observed"]["sequence"] = 101
    arm64["observed"]["digest_after"] = 4
    arm64["timestamp_unix_ms"] = 1784548800102

    with tempfile.TemporaryDirectory(prefix="rafz-selection-") as temporary:
        evidence = pathlib.Path(temporary) / "evidence"
        evidence.mkdir()
        arm32_bundle = matrix_helper.write_source(builder, evidence, "src-arm32", arm32, "SELECT-ARM32")
        arm64_bundle = matrix_helper.write_source(builder, evidence, "src-arm64", arm64, "SELECT-ARM64")

        partial = rebuild(evidence, "arm32-legacy", arm32_bundle)
        require(partial["state"] == "PARTIAL_DEVICE_PROOF", "selection partial state mismatch")
        require(partial["selected_bundles"] == {"arm32-legacy": arm32_bundle.name},
                "ARM32 selection mismatch")

        complete = rebuild(evidence, "arm64-modern", arm64_bundle)
        require(complete["state"] == "DUAL_ARM_DEVICE_PROOF", "selection complete state mismatch")
        require(complete["claim_allowed_device_matrix"] is True,
                "complete selected matrix did not promote claim")
        require((evidence / "selected-arm32-legacy.txt").read_text(encoding="utf-8").strip()
                == arm32_bundle.name, "ARM32 pointer changed unexpectedly")

        unsafe = evidence / "selected-x86.txt"
        unsafe.write_text("../escape\n", encoding="utf-8")
        try:
            rebuild(evidence)
        except SelectionError:
            pass
        else:
            raise AssertionError("unsafe selection traversal was accepted")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--evidence-root", type=pathlib.Path)
    parser.add_argument("--select-role", choices=ROLES)
    parser.add_argument("--select-bundle", type=pathlib.Path)
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args()

    if args.self_test:
        try:
            self_test()
        except (AssertionError, SelectionError, OSError, ValueError) as exc:
            print(f"RAFAELIA_ZERO_DEVICE_SELECTION_SELF_TEST=FAIL: {exc}", file=sys.stderr)
            return 1
        print("RAFAELIA_ZERO_DEVICE_SELECTION_SELF_TEST=PASS")
        return 0

    if args.evidence_root is None:
        parser.error("--evidence-root is required")
    try:
        matrix = rebuild(args.evidence_root, args.select_role, args.select_bundle)
    except (SelectionError, OSError, ValueError) as exc:
        print(f"RAFAELIA_ZERO_DEVICE_SELECTION=FAIL: {exc}", file=sys.stderr)
        return 1
    print(json.dumps(matrix, sort_keys=True, separators=(",", ":")))
    print(f"RAFAELIA_ZERO_DEVICE_SELECTION={matrix['state']}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
