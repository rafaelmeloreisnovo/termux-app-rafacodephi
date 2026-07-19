#!/usr/bin/env python3
"""Validate the RAFCODE-Phi native diagnostic/dead-code contract."""

from __future__ import annotations

import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

CHECKS = {
    "rafaelia/src/main/cpp/Android.mk": (
        "-ffunction-sections",
        "-fdata-sections",
        "-fno-common",
        "-Wno-error=unused-function",
        "-Wl,--gc-sections",
    ),
    "rmr/src/main/cpp/Android.mk": (
        "-ffunction-sections",
        "-fdata-sections",
        "-fno-common",
        "-Wl,--gc-sections",
    ),
    "app/src/main/cpp/Android.mk": (
        "-ffunction-sections",
        "-fdata-sections",
        "-fno-common",
        "-Wl,--gc-sections",
    ),
    "rafaelia/src/main/cpp/raf_compile_contract.h": (
        "RAF_UNUSED",
        "RAF_USED",
        "RAF_NORETURN",
        "RAF_DISCARD",
        "RAF_SPIN_FOREVER",
        "--gc-sections",
    ),
    "rafaelia/src/main/cpp/raf_numbase.c": (
        '#include "raf_compile_contract.h"',
        "Single linear pass",
        "i <= n / i",
        "0ULL - (unsigned long long)n",
    ),
}

FORBIDDEN = {
    "rafaelia/src/main/cpp/Android.mk": (
        "-Wno-unused-function",
    ),
}


def main() -> int:
    results: list[dict[str, object]] = []
    passed = True

    for relative, required in CHECKS.items():
        path = ROOT / relative
        if not path.is_file():
            results.append({"path": relative, "status": "FAIL", "reason": "missing file"})
            passed = False
            continue

        text = path.read_text(encoding="utf-8")
        missing = [token for token in required if token not in text]
        forbidden = [token for token in FORBIDDEN.get(relative, ()) if token in text]
        status = "PASS" if not missing and not forbidden else "FAIL"
        passed = passed and status == "PASS"
        results.append(
            {
                "path": relative,
                "status": status,
                "missing": missing,
                "forbidden": forbidden,
            }
        )

    report = {
        "schema": "raf.native-gc-contract.v1",
        "status": "PASS" if passed else "FAIL",
        "checks": results,
        "invariant": (
            "warnings remain visible; intentional exceptions are explicit; "
            "dead symbols are removed only by section-level reachability"
        ),
    }
    print(json.dumps(report, ensure_ascii=False, indent=2))
    return 0 if passed else 1


if __name__ == "__main__":
    raise SystemExit(main())
