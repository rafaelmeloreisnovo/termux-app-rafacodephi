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

ROOT = Path(__file__).resolve().parents[1]
RAFAELIA_C = ROOT / "rafaelia" / "src" / "main" / "cpp" / "rafaelia.c"
INDEX = ROOT / "rafaelia" / "termux-packages-manifests" / "INDEX.rafidx"
CORE_PKG = ROOT / "rafaelia" / "termux-packages-manifests" / "rafacodephi-core.rafpkg"
GC_CONTRACT = ROOT / "scripts" / "validate_raf_native_gc_contract.py"
WARNING_TESTS = ROOT / "tests" / "test_raf_compile_warning_contract.py"

FAILURES: list[str] = []


def require(condition: bool, message: str) -> None:
    if not condition:
        FAILURES.append(message)


def run_python_gate(path: Path, label: str) -> None:
    require(path.exists(), f"missing {label}: {path}")
    if not path.exists():
        return

    completed = subprocess.run(
        [sys.executable, str(path)],
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

    # Compose the new compiler-intent contract into the already-canonical
    # native-safety gate without adding another workflow definition.
    run_python_gate(GC_CONTRACT, "native-gc-contract")
    run_python_gate(WARNING_TESTS, "compile-warning-contract-tests")

    if FAILURES:
        print("RAFAELIA_NATIVE_SAFETY=fail")
        for item in FAILURES:
            print(f"- {item}")
        return 1

    print("RAFAELIA_NATIVE_SAFETY=pass")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
