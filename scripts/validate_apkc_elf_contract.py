#!/usr/bin/env python3
"""Independent fail-closed validator for APKC ELF emitters.

Accepted scopes:
- rel: little-endian ET_REL with exactly one null section header;
- exec: little-endian ET_EXEC with one RX PT_LOAD and a fixed exit(0) stub.

Passing the executable-structure contract does not prove physical Android
execution, dynamic linking, arbitrary code generation or a complete linker.
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
ET_EXEC = 2
EM_ARM = 40
EM_AARCH64 = 183
EF_ARM_EABI5 = 0x05000000
PT_LOAD = 1
PF_X = 1
PF_R = 4
PAGE_ALIGN = 0x1000
EXEC_CODE_OFFSET = 0x100
EXEC_SIZE = 0x10C
ARM32_BASE = 0x00010000
ARM64_BASE = 0x0000000000400000
ARM32_CODE = struct.pack("<III", 0xE3A07001, 0xE3A00000, 0xEF000000)
ARM64_CODE = struct.pack("<III", 0xD2800BA8, 0xD2800000, 0xD4000001)


def unpack(fmt: str, data: bytes, offset: int) -> tuple[int, ...]:
    size = struct.calcsize(fmt)
    if offset < 0 or offset + size > len(data):
        raise ValueError(f"out-of-bounds field at offset {offset} size {size}")
    return struct.unpack_from(fmt, data, offset)


def base_report(path: Path, expected: str, kind: str) -> tuple[bytes, list[str], list[dict[str, object]]]:
    data = path.read_bytes()
    failures: list[str] = []
    checks: list[dict[str, object]] = []

    def check(name: str, condition: bool, detail: object) -> None:
        checks.append({"name": name, "status": "PASS" if condition else "FAIL", "detail": detail})
        if not condition:
            failures.append(name)

    check("magic", data[:4] == ELF_MAGIC, data[:4].hex())
    if len(data) >= 16:
        expected_class = 1 if expected == "arm32" else 2
        check("class", data[4] == expected_class, data[4])
        check("little_endian", data[5] == 1, data[5])
        check("ident_version", data[6] == EV_CURRENT, data[6])
        check("sysv_osabi", data[7] == 0 and data[8] == 0,
              {"osabi": data[7], "abi_version": data[8]})
    else:
        failures.append("truncated_ident")

    return data, failures, checks


def parse_header(data: bytes, expected: str) -> dict[str, int]:
    if expected == "arm32":
        if len(data) < 52:
            raise ValueError("truncated ELF32 header")
        e_type, e_machine, e_version = unpack("<HHI", data, 16)
        e_entry, e_phoff, e_shoff, e_flags = unpack("<IIII", data, 24)
        e_ehsize, e_phentsize, e_phnum, e_shentsize, e_shnum, e_shstrndx = unpack("<HHHHHH", data, 40)
    else:
        if len(data) < 64:
            raise ValueError("truncated ELF64 header")
        e_type, e_machine, e_version = unpack("<HHI", data, 16)
        e_entry, e_phoff, e_shoff = unpack("<QQQ", data, 24)
        (e_flags,) = unpack("<I", data, 48)
        e_ehsize, e_phentsize, e_phnum, e_shentsize, e_shnum, e_shstrndx = unpack("<HHHHHH", data, 52)
    return {
        "type": e_type,
        "machine": e_machine,
        "version": e_version,
        "entry": e_entry,
        "phoff": e_phoff,
        "shoff": e_shoff,
        "flags": e_flags,
        "ehsize": e_ehsize,
        "phentsize": e_phentsize,
        "phnum": e_phnum,
        "shentsize": e_shentsize,
        "shnum": e_shnum,
        "shstrndx": e_shstrndx,
    }


def validate_rel(path: Path, expected: str) -> dict[str, object]:
    data, failures, checks = base_report(path, expected, "rel")

    def check(name: str, condition: bool, detail: object) -> None:
        checks.append({"name": name, "status": "PASS" if condition else "FAIL", "detail": detail})
        if not condition:
            failures.append(name)

    header = parse_header(data, expected)
    expected_machine = EM_ARM if expected == "arm32" else EM_AARCH64
    header_size = 52 if expected == "arm32" else 64
    section_size = 40 if expected == "arm32" else 64
    total_size = header_size + section_size
    expected_flags = EF_ARM_EABI5 if expected == "arm32" else 0

    check("exact_size", len(data) == total_size, {"actual": len(data), "expected": total_size})
    check("relocatable_type", header["type"] == ET_REL, header["type"])
    check("machine", header["machine"] == expected_machine, header["machine"])
    check("header_version", header["version"] == EV_CURRENT, header["version"])
    check("no_entrypoint", header["entry"] == 0, header["entry"])
    check("no_program_headers", header["phoff"] == 0 and header["phentsize"] == 0 and header["phnum"] == 0,
          {"phoff": header["phoff"], "phentsize": header["phentsize"], "phnum": header["phnum"]})
    check("section_table_offset", header["shoff"] == header_size, header["shoff"])
    check("header_size", header["ehsize"] == header_size, header["ehsize"])
    check("section_header_size", header["shentsize"] == section_size, header["shentsize"])
    check("null_section_only", header["shnum"] == 1 and header["shstrndx"] == 0,
          {"shnum": header["shnum"], "shstrndx": header["shstrndx"]})
    check("abi_flags", header["flags"] == expected_flags,
          {"actual": header["flags"], "expected": expected_flags})
    check("null_section_zeroed", data[header_size:total_size] == bytes(section_size),
          data[header_size:total_size].hex())

    return {
        "schema": "raf.apkc-elf-contract.v2",
        "path": str(path),
        "expected": expected,
        "kind": "rel",
        "status": "PASS" if not failures else "FAIL",
        "claim_allowed": not failures,
        "scope": "STRUCTURAL_ET_REL_ONLY",
        "limitations": ["no PT_LOAD segments", "no executable code", "no symbols", "no relocations", "no runtime proof"],
        "checks": checks,
        "failures": failures,
    }


def validate_exec(path: Path, expected: str) -> dict[str, object]:
    data, failures, checks = base_report(path, expected, "exec")

    def check(name: str, condition: bool, detail: object) -> None:
        checks.append({"name": name, "status": "PASS" if condition else "FAIL", "detail": detail})
        if not condition:
            failures.append(name)

    header = parse_header(data, expected)
    is_arm32 = expected == "arm32"
    expected_machine = EM_ARM if is_arm32 else EM_AARCH64
    header_size = 52 if is_arm32 else 64
    phdr_size = 32 if is_arm32 else 56
    base = ARM32_BASE if is_arm32 else ARM64_BASE
    expected_flags = EF_ARM_EABI5 if is_arm32 else 0
    expected_code = ARM32_CODE if is_arm32 else ARM64_CODE

    check("exact_size", len(data) == EXEC_SIZE, {"actual": len(data), "expected": EXEC_SIZE})
    check("executable_type", header["type"] == ET_EXEC, header["type"])
    check("machine", header["machine"] == expected_machine, header["machine"])
    check("header_version", header["version"] == EV_CURRENT, header["version"])
    check("entrypoint", header["entry"] == base + EXEC_CODE_OFFSET,
          {"actual": header["entry"], "expected": base + EXEC_CODE_OFFSET})
    check("program_header_table", header["phoff"] == header_size
          and header["phentsize"] == phdr_size and header["phnum"] == 1,
          {"phoff": header["phoff"], "phentsize": header["phentsize"], "phnum": header["phnum"]})
    check("no_section_table", header["shoff"] == 0 and header["shentsize"] == 0
          and header["shnum"] == 0 and header["shstrndx"] == 0,
          {"shoff": header["shoff"], "shentsize": header["shentsize"],
           "shnum": header["shnum"], "shstrndx": header["shstrndx"]})
    check("header_size", header["ehsize"] == header_size, header["ehsize"])
    check("abi_flags", header["flags"] == expected_flags,
          {"actual": header["flags"], "expected": expected_flags})

    if is_arm32:
        p_type, p_offset, p_vaddr, p_paddr, p_filesz, p_memsz, p_flags, p_align = unpack("<IIIIIIII", data, header_size)
    else:
        p_type, p_flags, p_offset, p_vaddr, p_paddr, p_filesz, p_memsz, p_align = unpack("<IIQQQQQQ", data, header_size)

    check("load_segment_type", p_type == PT_LOAD, p_type)
    check("load_segment_rx", p_flags == (PF_R | PF_X), p_flags)
    check("load_segment_offset", p_offset == 0, p_offset)
    check("load_segment_address", p_vaddr == base and p_paddr == base,
          {"vaddr": p_vaddr, "paddr": p_paddr, "expected": base})
    check("load_segment_size", p_filesz == EXEC_SIZE and p_memsz == EXEC_SIZE,
          {"filesz": p_filesz, "memsz": p_memsz, "expected": EXEC_SIZE})
    check("load_segment_alignment", p_align == PAGE_ALIGN and p_vaddr % p_align == p_offset % p_align,
          {"align": p_align, "vaddr_mod": p_vaddr % p_align if p_align else None,
           "offset_mod": p_offset % p_align if p_align else None})
    check("entry_inside_load_segment", p_vaddr <= header["entry"] < p_vaddr + p_memsz,
          {"entry": header["entry"], "start": p_vaddr, "end": p_vaddr + p_memsz})
    check("zero_padding_before_code", data[header_size + phdr_size:EXEC_CODE_OFFSET] == bytes(EXEC_CODE_OFFSET - header_size - phdr_size),
          {"start": header_size + phdr_size, "end": EXEC_CODE_OFFSET})
    check("fixed_exit_stub", data[EXEC_CODE_OFFSET:EXEC_CODE_OFFSET + len(expected_code)] == expected_code,
          data[EXEC_CODE_OFFSET:EXEC_CODE_OFFSET + len(expected_code)].hex())

    return {
        "schema": "raf.apkc-elf-contract.v2",
        "path": str(path),
        "expected": expected,
        "kind": "exec",
        "status": "PASS" if not failures else "FAIL",
        "claim_allowed": not failures,
        "scope": "MINIMAL_STATIC_EXIT_STUB_STRUCTURE",
        "limitations": [
            "fixed exit(0) payload only",
            "no section table",
            "no symbols",
            "no relocations",
            "no dynamic linking",
            "not an arbitrary-code linker",
            "no physical-device runtime proof",
        ],
        "checks": checks,
        "failures": failures,
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("path", type=Path)
    parser.add_argument("--expect", choices=("arm32", "arm64"), required=True)
    parser.add_argument("--kind", choices=("rel", "exec"), default="rel")
    parser.add_argument("--pretty", action="store_true")
    args = parser.parse_args()

    try:
        report = validate_exec(args.path, args.expect) if args.kind == "exec" else validate_rel(args.path, args.expect)
    except (OSError, ValueError, struct.error) as exc:
        report = {
            "schema": "raf.apkc-elf-contract.v2",
            "path": str(args.path),
            "expected": args.expect,
            "kind": args.kind,
            "status": "FAIL",
            "claim_allowed": False,
            "error": str(exc),
        }

    json.dump(report, sys.stdout, ensure_ascii=False, indent=2 if args.pretty else None)
    sys.stdout.write("\n")
    return 0 if report.get("status") == "PASS" else 1


if __name__ == "__main__":
    raise SystemExit(main())
