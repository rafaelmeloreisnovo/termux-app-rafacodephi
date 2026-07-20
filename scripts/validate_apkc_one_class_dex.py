#!/usr/bin/env python3
"""Independent validator for APKC's one-class DEX 035 fixture.

Expected semantic body:
    public class Lraf/apkc/Stub; extends Ljava/lang/Object;
    public static run()V { return-void }

Passing this gate proves a bounded DEX table/code fixture, not a Java/Kotlin
compiler, verifier-complete arbitrary bytecode backend, APK runtime or multidex.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import struct
import sys
import zlib
from pathlib import Path

MAGIC = b"dex\n035\x00"
ENDIAN_TAG = 0x12345678
NO_INDEX = 0xFFFFFFFF
ACC_PUBLIC = 0x1
ACC_STATIC = 0x8
RETURN_VOID = 0x000E

EXPECTED_STRINGS = ["Ljava/lang/Object;", "Lraf/apkc/Stub;", "V", "run"]
EXPECTED_MAP = [
    (0x0000, 1, 0x000),
    (0x0001, 4, 0x070),
    (0x0002, 3, 0x080),
    (0x0003, 1, 0x08C),
    (0x0005, 1, 0x098),
    (0x0006, 1, 0x0A0),
    (0x2002, 4, 0x0C0),
    (0x2000, 1, 0x0ED),
    (0x2001, 1, 0x0F8),
    (0x1000, 1, 0x10C),
]


def take(data: bytes, offset: int, size: int) -> bytes:
    if offset < 0 or size < 0 or offset + size > len(data):
        raise ValueError(f"out-of-bounds offset={offset} size={size} file={len(data)}")
    return data[offset:offset + size]


def u16(data: bytes, offset: int) -> int:
    return struct.unpack_from("<H", take(data, offset, 2))[0]


def u32(data: bytes, offset: int) -> int:
    return struct.unpack_from("<I", take(data, offset, 4))[0]


def uleb128(data: bytes, offset: int) -> tuple[int, int]:
    value = 0
    shift = 0
    cursor = offset
    for _ in range(5):
        byte = take(data, cursor, 1)[0]
        cursor += 1
        value |= (byte & 0x7F) << shift
        if byte & 0x80 == 0:
            return value, cursor
        shift += 7
    raise ValueError(f"invalid ULEB128 at {offset}")


def string_data(data: bytes, offset: int) -> tuple[str, int]:
    utf16_size, cursor = uleb128(data, offset)
    end = data.find(b"\x00", cursor)
    if end < 0:
        raise ValueError(f"unterminated string_data_item at {offset}")
    raw = take(data, cursor, end - cursor)
    text = raw.decode("utf-8")
    if len(text) != utf16_size:
        raise ValueError(f"UTF-16 length mismatch at {offset}: {utf16_size} != {len(text)}")
    return text, end + 1


def validate(path: Path) -> dict[str, object]:
    data = path.read_bytes()
    failures: list[str] = []
    checks: list[dict[str, object]] = []

    def check(name: str, condition: bool, detail: object) -> None:
        checks.append({"name": name, "status": "PASS" if condition else "FAIL", "detail": detail})
        if not condition:
            failures.append(name)

    check("magic", data[:8] == MAGIC, data[:8].hex())
    if len(data) < 0x70:
        raise ValueError("truncated DEX header")

    file_size = u32(data, 32)
    header_size = u32(data, 36)
    endian_tag = u32(data, 40)
    map_off = u32(data, 52)
    string_count, string_ids_off = u32(data, 56), u32(data, 60)
    type_count, type_ids_off = u32(data, 64), u32(data, 68)
    proto_count, proto_ids_off = u32(data, 72), u32(data, 76)
    field_count, field_ids_off = u32(data, 80), u32(data, 84)
    method_count, method_ids_off = u32(data, 88), u32(data, 92)
    class_count, class_defs_off = u32(data, 96), u32(data, 100)
    data_size, data_off = u32(data, 104), u32(data, 108)

    check("exact_file_size", len(data) == file_size == 0x188,
          {"actual": len(data), "header": file_size, "expected": 0x188})
    check("header_size", header_size == 0x70, header_size)
    check("endian_tag", endian_tag == ENDIAN_TAG, hex(endian_tag))
    check("link_section_absent", u32(data, 44) == 0 and u32(data, 48) == 0,
          {"link_size": u32(data, 44), "link_off": u32(data, 48)})
    check("data_bounds", data_off == 0x0C0 and data_size == file_size - data_off,
          {"data_off": data_off, "data_size": data_size})
    check("signature_sha1", data[12:32] == hashlib.sha1(data[32:]).digest(), data[12:32].hex())
    check("checksum_adler32", u32(data, 8) == (zlib.adler32(data[12:]) & 0xFFFFFFFF), hex(u32(data, 8)))

    check("string_table", (string_count, string_ids_off) == (4, 0x70),
          {"count": string_count, "offset": string_ids_off})
    check("type_table", (type_count, type_ids_off) == (3, 0x80),
          {"count": type_count, "offset": type_ids_off})
    check("proto_table", (proto_count, proto_ids_off) == (1, 0x8C),
          {"count": proto_count, "offset": proto_ids_off})
    check("field_table_absent", field_count == 0 and field_ids_off == 0,
          {"count": field_count, "offset": field_ids_off})
    check("method_table", (method_count, method_ids_off) == (1, 0x98),
          {"count": method_count, "offset": method_ids_off})
    check("class_table", (class_count, class_defs_off) == (1, 0xA0),
          {"count": class_count, "offset": class_defs_off})

    string_offsets = [u32(data, string_ids_off + 4 * index) for index in range(string_count)]
    parsed_strings = [string_data(data, offset)[0] for offset in string_offsets]
    check("strings_exact", parsed_strings == EXPECTED_STRINGS, parsed_strings)
    check("strings_sorted", parsed_strings == sorted(parsed_strings), parsed_strings)

    descriptor_indices = [u32(data, type_ids_off + 4 * index) for index in range(type_count)]
    check("type_descriptor_indices", descriptor_indices == [0, 1, 2], descriptor_indices)
    check("type_descriptors", [parsed_strings[index] for index in descriptor_indices]
          == EXPECTED_STRINGS[:3], [parsed_strings[index] for index in descriptor_indices])

    proto = (u32(data, proto_ids_off), u32(data, proto_ids_off + 4), u32(data, proto_ids_off + 8))
    check("void_noarg_proto", proto == (2, 2, 0), proto)

    method = (u16(data, method_ids_off), u16(data, method_ids_off + 2), u32(data, method_ids_off + 4))
    check("stub_run_method_id", method == (1, 0, 3), method)

    class_values = tuple(u32(data, class_defs_off + 4 * index) for index in range(8))
    class_idx, class_access, superclass_idx, interfaces_off, source_idx, annotations_off, class_data_off, static_values_off = class_values
    check("class_descriptor", parsed_strings[descriptor_indices[class_idx]] == "Lraf/apkc/Stub;", class_idx)
    check("class_public", class_access == ACC_PUBLIC, class_access)
    check("class_super_object", parsed_strings[descriptor_indices[superclass_idx]] == "Ljava/lang/Object;", superclass_idx)
    check("class_optional_sections_absent",
          interfaces_off == 0 and source_idx == NO_INDEX and annotations_off == 0 and static_values_off == 0,
          {"interfaces": interfaces_off, "source": source_idx, "annotations": annotations_off, "static": static_values_off})
    check("class_data_offset", class_data_off == 0x0ED, class_data_off)

    cursor = class_data_off
    static_fields, cursor = uleb128(data, cursor)
    instance_fields, cursor = uleb128(data, cursor)
    direct_methods, cursor = uleb128(data, cursor)
    virtual_methods, cursor = uleb128(data, cursor)
    method_idx_diff, cursor = uleb128(data, cursor)
    method_access, cursor = uleb128(data, cursor)
    code_off, cursor = uleb128(data, cursor)
    check("class_data_counts", (static_fields, instance_fields, direct_methods, virtual_methods) == (0, 0, 1, 0),
          (static_fields, instance_fields, direct_methods, virtual_methods))
    check("encoded_method", method_idx_diff == 0 and method_access == (ACC_PUBLIC | ACC_STATIC) and code_off == 0x0F8,
          {"method_idx_diff": method_idx_diff, "access": method_access, "code_off": code_off})
    check("code_alignment", code_off % 4 == 0, code_off)

    registers, ins, outs, tries = (u16(data, code_off + 2 * index) for index in range(4))
    debug_info_off = u32(data, code_off + 8)
    insns_size = u32(data, code_off + 12)
    instruction = u16(data, code_off + 16)
    check("code_frame", (registers, ins, outs, tries, debug_info_off) == (0, 0, 0, 0, 0),
          {"registers": registers, "ins": ins, "outs": outs, "tries": tries, "debug": debug_info_off})
    check("single_return_void", insns_size == 1 and instruction == RETURN_VOID,
          {"insns_size": insns_size, "instruction": hex(instruction)})

    map_count = u32(data, map_off)
    map_entries: list[tuple[int, int, int]] = []
    for index in range(map_count):
        offset = map_off + 4 + 12 * index
        item_type = u16(data, offset)
        unused = u16(data, offset + 2)
        item_size = u32(data, offset + 4)
        item_off = u32(data, offset + 8)
        check(f"map_unused_zero_{index}", unused == 0, unused)
        map_entries.append((item_type, item_size, item_off))
    check("map_entries_exact", map_entries == EXPECTED_MAP, map_entries)
    check("map_offsets_sorted", [entry[2] for entry in map_entries] == sorted(entry[2] for entry in map_entries),
          [entry[2] for entry in map_entries])
    check("map_consumes_file", map_off + 4 + map_count * 12 == len(data),
          {"map_end": map_off + 4 + map_count * 12, "file_size": len(data)})

    return {
        "schema": "raf.apkc-one-class-dex-contract.v1",
        "path": str(path),
        "status": "PASS" if not failures else "FAIL",
        "claim_allowed": not failures,
        "scope": "ONE_CLASS_ONE_STATIC_RETURN_VOID_METHOD",
        "class_descriptor": "Lraf/apkc/Stub;",
        "method": "run()V",
        "limitations": [
            "fixed class and method only",
            "no constructor",
            "no fields",
            "no arbitrary instructions",
            "no annotations or debug info",
            "no exception handlers",
            "no multidex",
            "not a Java or Kotlin compiler",
            "no ART or Dalvik runtime proof",
        ],
        "checks": checks,
        "failures": failures,
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("path", type=Path)
    parser.add_argument("--pretty", action="store_true")
    args = parser.parse_args()

    try:
        report = validate(args.path)
    except (OSError, UnicodeDecodeError, ValueError, struct.error, IndexError) as exc:
        report = {
            "schema": "raf.apkc-one-class-dex-contract.v1",
            "path": str(args.path),
            "status": "FAIL",
            "claim_allowed": False,
            "error": str(exc),
        }

    json.dump(report, sys.stdout, ensure_ascii=False, indent=2 if args.pretty else None)
    sys.stdout.write("\n")
    return 0 if report.get("status") == "PASS" else 1


if __name__ == "__main__":
    raise SystemExit(main())
