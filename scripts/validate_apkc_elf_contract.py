#!/usr/bin/env python3
"""Independent fail-closed validator for APKC structural ELF emitters.

The accepted scope is deliberately narrow: little-endian ET_REL for ARM or
AArch64, no program headers, and exactly one all-zero null section header.
Passing this validator does not prove executable output, linking or runtime.
"""

from __future__ import annotations

import argparse
import json
import struct
import sys
from pathlib import Path

ELF_MAGIC = b"\x7fELF"
EV_CURRENT = 1
ET_REL = 1
EM_ARM = 40
EM_AARCH64 = 183
EF_ARM_EABI5 = 0x05000000


def unpack(fmt: str, data: bytes, offset: int) -> tuple[int, ...]:
    size = struct.calcsize(fmt)
    if offset < 0 or offset + size > len(data):
        raise ValueError(f"out-of-bounds field at offset {offset} size {size}")
    return struct.unpack_from(fmt, data, offset)


def validate(path: Path, expected: str) -> dict[str, object]:
    data = path.read_bytes()
    failures: list[str] = []
    checks: list[dict[str, object]] = []

    def check(name: str, condition: bool, detail: object) -> None:
        checks.append({"name": name, "status": "PASS" if condition else "FAIL", "detail": detail})
        if not condition:
            failures.append(name)

    check("magic", data[:4] == ELF_MAGIC, data[:4].hex())
    if len(data) < 16 or data[:4] != ELF_MAGIC:
        return {
            "schema": "raf.apkc-elf-contract.v1",
            "path": str(path),
            "expected": expected,
            "status": "FAIL",
            "claim_allowed": False,
            "scope": "STRUCTURAL_ET_REL_ONLY",
            "checks": checks,
            "failures": failures or ["truncated_ident"],
        }

    elf_class = data[4]
    data_encoding = data[5]
    ident_version = data[6]
    osabi = data[7]
    abi_version = data[8]

    expected_class = 1 if expected == "arm32" else 2
    expected_machine = EM_ARM if expected == "arm32" else EM_AARCH64
    expected_header_size = 52 if expected == "arm32" else 64
    expected_section_size = 40 if expected == "arm32" else 64
    expected_total = expected_header_size + expected_section_size

    check("class", elf_class == expected_class, elf_class)
    check("little_endian", data_encoding == 1, data_encoding)
    check("ident_version", ident_version == EV_CURRENT, ident_version)
    check("sysv_osabi", osabi == 0 and abi_version == 0, {"osabi": osabi, "abi_version": abi_version})
    check("exact_size", len(data) == expected_total, {"actual": len(data), "expected": expected_total})

    if elf_class == 1 and len(data) >= 52:
        e_type, e_machine, e_version = unpack("<HHI", data, 16)
        e_entry, e_phoff, e_shoff, e_flags = unpack("<IIII", data, 24)
        e_ehsize, e_phentsize, e_phnum, e_shentsize, e_shnum, e_shstrndx = unpack("<HHHHHH", data, 40)
    elif elf_class == 2 and len(data) >= 64:
        e_type, e_machine, e_version = unpack("<HHI", data, 16)
        e_entry, e_phoff, e_shoff = unpack("<QQQ", data, 24)
        (e_flags,) = unpack("<I", data, 48)
        e_ehsize, e_phentsize, e_phnum, e_shentsize, e_shnum, e_shstrndx = unpack("<HHHHHH", data, 52)
    else:
        failures.append("truncated_or_unknown_header")
        return {
            "schema": "raf.apkc-elf-contract.v1",
            "path": str(path),
            "expected": expected,
            "status": "FAIL",
            "claim_allowed": False,
            "scope": "STRUCTURAL_ET_REL_ONLY",
            "checks": checks,
            "failures": failures,
        }

    check("relocatable_type", e_type == ET_REL, e_type)
    check("machine", e_machine == expected_machine, e_machine)
    check("header_version", e_version == EV_CURRENT, e_version)
    check("no_entrypoint", e_entry == 0, e_entry)
    check("no_program_headers", e_phoff == 0 and e_phentsize == 0 and e_phnum == 0,
          {"phoff": e_phoff, "phentsize": e_phentsize, "phnum": e_phnum})
    check("section_table_offset", e_shoff == expected_header_size, e_shoff)
    check("header_size", e_ehsize == expected_header_size, e_ehsize)
    check("section_header_size", e_shentsize == expected_section_size, e_shentsize)
    check("null_section_only", e_shnum == 1 and e_shstrndx == 0,
          {"shnum": e_shnum, "shstrndx": e_shstrndx})

    expected_flags = EF_ARM_EABI5 if expected == "arm32" else 0
    check("abi_flags", e_flags == expected_flags, {"actual": e_flags, "expected": expected_flags})

    null_start = expected_header_size
    null_end = null_start + expected_section_size
    check("null_section_zeroed", data[null_start:null_end] == bytes(expected_section_size),
          data[null_start:null_end].hex())

    return {
        "schema": "raf.apkc-elf-contract.v1",
        "path": str(path),
        "expected": expected,
        "status": "PASS" if not failures else "FAIL",
        "claim_allowed": not failures,
        "scope": "STRUCTURAL_ET_REL_ONLY",
        "limitations": [
            "no PT_LOAD segments",
            "no executable code",
            "no symbols",
            "no relocations",
            "no dynamic linking",
            "no runtime proof",
        ],
        "checks": checks,
        "failures": failures,
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("path", type=Path)
    parser.add_argument("--expect", choices=("arm32", "arm64"), required=True)
    parser.add_argument("--pretty", action="store_true")
    args = parser.parse_args()

    try:
        report = validate(args.path, args.expect)
    except (OSError, ValueError, struct.error) as exc:
        report = {
            "schema": "raf.apkc-elf-contract.v1",
            "path": str(args.path),
            "expected": args.expect,
            "status": "FAIL",
            "claim_allowed": False,
            "scope": "STRUCTURAL_ET_REL_ONLY",
            "error": str(exc),
        }

    json.dump(report, sys.stdout, ensure_ascii=False, indent=2 if args.pretty else None)
    sys.stdout.write("\n")
    return 0 if report.get("status") == "PASS" else 1


if __name__ == "__main__":
    raise SystemExit(main())
