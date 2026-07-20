#!/usr/bin/env python3
"""Verify Rafaelia native safety contracts.

This script is intentionally static and conservative. It does not claim the
native runtime is correct; it blocks known unsafe/stub patterns so CI can fail
before shipping incomplete JNI code.
"""
from __future__ import annotations

import subprocess
import sys
from pathlib import Path
from typing import Sequence

ROOT = Path(__file__).resolve().parents[1]
RAFAELIA_C = ROOT / "rafaelia" / "src" / "main" / "cpp" / "rafaelia.c"
INDEX = ROOT / "rafaelia" / "termux-packages-manifests" / "INDEX.rafidx"
CORE_PKG = ROOT / "rafaelia" / "termux-packages-manifests" / "rafacodephi-core.rafpkg"
NATIVE_COMPILE_GATE = ROOT / "scripts" / "test_raf_native_compile_contract.sh"
ZERO_RUNTIME_GATE = ROOT / "scripts" / "validate_rafaelia_zero_runtime.py"
SYSTEM_FINALIZATION_GATE = ROOT / "tools" / "validate_system_finalization.py"
SYSTEM_FINALIZATION_TESTS = ROOT / "tests" / "test_system_finalization.py"

FAILURES: list[str] = []


def require(condition: bool, message: str) -> None:
    if not condition:
        FAILURES.append(message)


def run_gate(command: Sequence[str], label: str) -> None:
    completed = subprocess.run(
        list(command),
        cwd=ROOT,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        check=False,
    )
    output = completed.stdout.strip()
    if output:
        print(f"[{label}]")
        print(output)
    require(completed.returncode == 0, f"{label} failed with exit={completed.returncode}")


def main() -> int:
    require(RAFAELIA_C.exists(), f"missing native file: {RAFAELIA_C}")
    if RAFAELIA_C.exists():
        text = RAFAELIA_C.read_text(encoding="utf-8")
        require("RafaeliaVAContext" in text, "initVA must allocate a real RafaeliaVAContext")
        require("return 0; // Stub implementation" not in text, "initVA still returns the explicit stub handle")
        require("GetArrayLength(env, dest)" in text, "memcpy must bound n by destination array length")
        require("GetArrayLength(env, src)" in text, "memcpy must bound n by source array length")
        require("GetArrayLength(env, array)" in text, "memset must bound n by array length")
        require(
            "fabsf(denom)" in text or "denom" in text and "1e-10f" in text,
            "fitLeastSquares must guard denominator zero",
        )
        require("free(ptr)" in text or "free(ctx" in text, "releaseVA must free allocated native context")

    require(INDEX.exists(), f"missing package index: {INDEX}")
    if INDEX.exists():
        idx = INDEX.read_text(encoding="utf-8")
        require("local_packages=" in idx, "INDEX.rafidx must record local_packages")
        require("exported_packages=701" in idx, "INDEX.rafidx must include local package count")

    require(CORE_PKG.exists(), f"missing local package manifest: {CORE_PKG}")
    if CORE_PKG.exists():
        pkg = CORE_PKG.read_text(encoding="utf-8")
        require("seal=RAFPKG" in pkg, "rafacodephi-core manifest missing RAFPkg seal")
        require("name=rafacodephi-core" in pkg, "rafacodephi-core manifest missing canonical name")

    require(NATIVE_COMPILE_GATE.exists(), f"missing native compile gate: {NATIVE_COMPILE_GATE}")
    if NATIVE_COMPILE_GATE.exists():
        run_gate(["bash", str(NATIVE_COMPILE_GATE)], "native-compile-contract")

    require(ZERO_RUNTIME_GATE.exists(), f"missing RAFAELIA ZERO gate: {ZERO_RUNTIME_GATE}")
    if ZERO_RUNTIME_GATE.exists():
        run_gate([sys.executable, str(ZERO_RUNTIME_GATE)], "rafaelia-zero-runtime-contract")

    # The finalization unit tests prove that safe-core closes while release and
    # full-platform remain blocked by their own evidence requirements.
    require(SYSTEM_FINALIZATION_TESTS.exists(), f"missing system finalization tests: {SYSTEM_FINALIZATION_TESTS}")
    if SYSTEM_FINALIZATION_TESTS.exists():
        run_gate(
            [sys.executable, "-m", "unittest", "tests/test_system_finalization.py", "-v"],
            "system-finalization-tests",
        )

    # Close only the static/fail-closed implementation profile. This explicitly
    # does not promote functional distribution release, device proof, production
    # signing, TLS, complete compilers or a complete VM.
    require(SYSTEM_FINALIZATION_GATE.exists(), f"missing system finalization gate: {SYSTEM_FINALIZATION_GATE}")
    if SYSTEM_FINALIZATION_GATE.exists():
        run_gate(
            [
                sys.executable,
                str(SYSTEM_FINALIZATION_GATE),
                "--profile",
                "safe-core",
                "--strict",
                "--write-report",
            ],
            "system-finalization-safe-core",
        )

    if FAILURES:
        print("RAFAELIA_NATIVE_SAFETY=fail")
        for item in FAILURES:
            print(f"- {item}")
        return 1

    print("RAFAELIA_NATIVE_SAFETY=pass")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
