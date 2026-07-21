#!/usr/bin/env python3
"""Validate the fail-closed BITRAF eight-cycle temporal release contract."""
from __future__ import annotations

import argparse
import json
from pathlib import Path
from typing import Mapping

ROOT = Path(__file__).resolve().parents[1]
REPORT = Path("build/reports/raf-phase-release/validation.json")
FILES = {
    "contract": "configs/raf-phase-release-contract.json",
    "header": "rafaelia/src/main/cpp/raf_phase_release_gate.h",
    "source": "rafaelia/src/main/cpp/raf_phase_release_gate.c",
    "native_test": "tests/native/test_raf_phase_release_gate.c",
    "android_mk": "rafaelia/src/main/cpp/Android.mk",
    "compile_gate": "scripts/test_raf_native_compile_contract.sh",
}


def load(root: Path = ROOT) -> dict[str, str]:
    return {
        key: (root / relative).read_text(encoding="utf-8")
        for key, relative in FILES.items()
    }


def validate_snapshot(files: Mapping[str, str]) -> list[str]:
    errors: list[str] = []
    contract = json.loads(files["contract"])
    header = files["header"]
    source = files["source"]
    native_test = files["native_test"]
    android_mk = files["android_mk"]
    compile_gate = files["compile_gate"]

    expected = {
        "logical_period_us": 100000,
        "base_frequency_mhz": 10000,
        "frequency_step_mhz": 100,
        "max_frequency_mhz": 999000,
        "phases_per_cycle": 4,
        "window_cycles": 8,
        "phase_observations_per_release": 32,
        "nominal_window_us": 800000,
    }
    if contract.get("default_configuration") != expected:
        errors.append("default temporal configuration drift")
    if contract.get("claim_allowed") is not False or contract.get("release_allowed") is not False:
        errors.append("contract must keep claims and release disabled")

    current = contract.get("current_state", {})
    for field in (
        "android_build",
        "runtime_scheduler_integration",
        "physical_100ms_cadence",
        "arm32_device_receipt",
        "arm64_device_receipt",
    ):
        if current.get(field) != "TOKEN_VAZIO":
            errors.append(f"unproven operational field promoted: {field}")

    required_header = (
        "RAF_PHASE_GATE_LOGICAL_PERIOD_US 100000u",
        "RAF_PHASE_GATE_BASE_FREQUENCY_MHZ 10000u",
        "RAF_PHASE_GATE_STEP_MHZ 100u",
        "RAF_PHASE_GATE_MAX_FREQUENCY_MHZ 999000u",
        "RAF_PHASE_GATE_PHASES_PER_CYCLE 4u",
        "RAF_PHASE_GATE_WINDOW_CYCLES 8u",
        "RAF_PHASE_GATE_WINDOW_MASK 0xFFu",
        "RAF_PHASE_GATE_FLAG_ALIAS_ANCHOR",
        "RAF_PHASE_GATE_FLAG_FAULT_LATCHED",
        "raf_phase_release_gate_step",
        "raf_phase_release_gate_reset_fault",
    )
    for token in required_header:
        if token not in header:
            errors.append(f"header missing: {token}")

    required_source = (
        "_Static_assert(RAF_PHASE_GATE_PHASES_PER_CYCLE == 4u",
        "_Static_assert(RAF_PHASE_GATE_WINDOW_CYCLES == 8u",
        "gate->completed_mask != RAF_PHASE_GATE_WINDOW_MASK",
        "gate->released_digest = gate->staged_digest",
        "gate->release_epoch++",
        "raf_phase_gate_advance_frequency(gate)",
        "RAF_PHASE_GATE_ERR_PHASE_ORDER",
        "RAF_PHASE_GATE_ERR_EVIDENCE",
        "RAF_PHASE_GATE_FLAG_FAULT_LATCHED",
        "fractional == 0u",
        "RAF_PHASE_GATE_FLAG_ALIAS_ANCHOR",
    )
    for token in required_source:
        if token not in source:
            errors.append(f"source missing: {token}")

    forbidden_source = (
        "malloc(", "calloc(", "realloc(", "free(",
        "nanosleep(", "usleep(", "clock_gettime(",
        "float ", "double ", "sin(", "cos(",
    )
    for token in forbidden_source:
        if token in source:
            errors.append(f"forbidden kernel dependency: {token}")

    required_test = (
        "cycle < 7u",
        "complete_cycle(&gate, 7u, &event, 1)",
        "gate.release_epoch == 0u",
        "gate.release_epoch == 1u",
        "gate.frequency_mhz == 10100u",
        "RAF_PHASE_GATE_ERR_PHASE_ORDER",
        "RAF_PHASE_GATE_ERR_EVIDENCE",
        "RAF_PHASE_GATE_FLAG_ALIAS_ANCHOR",
    )
    for token in required_test:
        if token not in native_test:
            errors.append(f"native test missing: {token}")

    if "raf_phase_release_gate.c" not in android_mk:
        errors.append("Android.mk does not compile the phase release gate")
    for token in (
        "compile_phase_release_gate",
        "test_phase_release_gate",
        "validate_raf_phase_release_contract.py",
        "test_raf_phase_release_contract.py",
    ):
        if token not in compile_gate:
            errors.append(f"canonical native gate missing: {token}")

    release_position = source.find("gate->released_digest = gate->staged_digest")
    call_token = "raf_phase_gate_advance_frequency(gate)"
    call_positions: list[int] = []
    cursor = 0
    while True:
        position = source.find(call_token, cursor)
        if position < 0:
            break
        call_positions.append(position)
        cursor = position + len(call_token)
    if release_position < 0 or len(call_positions) != 1 or call_positions[0] < release_position:
        errors.append("frequency must advance only after atomic release")

    return errors


def build_report(root: Path = ROOT) -> dict[str, object]:
    errors = validate_snapshot(load(root))
    return {
        "schema": "rafcodephi.bitraf.phase-release-validation.v1",
        "status": "PASS" if not errors else "FAIL",
        "logical_period_us": 100000,
        "base_frequency_mhz": 10000,
        "frequency_step_mhz": 100,
        "phases_per_cycle": 4,
        "window_cycles": 8,
        "phase_observations_per_release": 32,
        "claim_allowed": False,
        "release_allowed": False,
        "physical_clock_proven": False,
        "errors": errors,
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--root", type=Path, default=ROOT)
    parser.add_argument("--strict", action="store_true")
    parser.add_argument("--write-report", action="store_true")
    args = parser.parse_args()

    report = build_report(args.root)
    text = json.dumps(report, ensure_ascii=False, sort_keys=True, indent=2) + "\n"
    if args.write_report:
        path = args.root / REPORT
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(text, encoding="utf-8")
    print(text, end="")
    return 1 if args.strict and report["status"] != "PASS" else 0


if __name__ == "__main__":
    raise SystemExit(main())
