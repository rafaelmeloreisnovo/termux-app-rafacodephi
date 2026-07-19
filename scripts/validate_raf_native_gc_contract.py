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
    "rafaelia/src/main/cpp/raf_ecc32_masked.h": (
        "_Static_assert(sizeof(unsigned int) == 4u",
        "RAF_ECC32_FORCE_COMPACT",
        "RAF_ECC32_FORCE_UNROLL",
        "__OPTIMIZE_SIZE__",
        "RAF_ECC32_MASK_0 0x55555555u",
        "RAF_ECC32_MASK_1 0x66666666u",
        "RAF_ECC32_MASK_2 0x78787878u",
        "RAF_ECC32_MASK_3 0x7F807F80u",
        "RAF_ECC32_MASK_4 0x7FFF8000u",
        "RAF_ECC32_MASK_5 0x80000000u",
        "raf_parity32_fold",
        "raf_ecc32_masked",
    ),
    "rafaelia/src/main/cpp/rafaelia_bitraf_core.c": (
        '#include "raf_ecc32_masked.h"',
        "return (u8)raf_ecc32_masked((unsigned int)v);",
        "sem loops aninhados",
    ),
    "tests/native/test_raf_ecc32_masked.c": (
        "32-element standard basis",
        "1000000u",
        "raf_ecc32_reference",
        "raf_ecc32_masked",
    ),
    "scripts/test_raf_native_compile_contract.sh": (
        "-DRAF_ECC32_FORCE_COMPACT=1",
        "-DRAF_ECC32_FORCE_UNROLL=1",
        "test_raf_ecc32_compact",
        "test_raf_ecc32_unrolled",
    ),
}

FORBIDDEN = {
    "rafaelia/src/main/cpp/Android.mk": (
        "-Wno-unused-function",
    ),
    "rafaelia/src/main/cpp/rafaelia_bitraf_core.c": (
        "for(u8 bit=0u; bit<6u; bit++)",
        "for(u8 i=0u; i<32u; i++)",
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
        "schema": "raf.native-gc-contract.v3",
        "status": "PASS" if passed else "FAIL",
        "checks": results,
        "invariant": (
            "warnings remain visible; intentional exceptions are explicit; "
            "dead symbols are removed only by section-level reachability; "
            "ECC32 runtime uses a basis-proven compile-time policy"
        ),
    }
    print(json.dumps(report, ensure_ascii=False, indent=2))
    return 0 if passed else 1


if __name__ == "__main__":
    raise SystemExit(main())
