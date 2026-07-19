#!/usr/bin/env python3
"""Independent validator for the APKC minimal DEX emitter."""

from __future__ import annotations

import argparse
import hashlib
import json
import struct
import sys
import zlib
from pathlib import Path

DEX_MINIMAL_SIZE = 0x8C
DEX_HEADER_SIZE = 0x70
DEX_ENDIAN_TAG = 0x12345678
DEX_TYPE_HEADER = 0x0000
DEX_TYPE_MAP_LIST = 0x1000


def u16(data: bytes, offset: int) -> int:
    return struct.unpack_from("<H", data, offset)[0]


def u32(data: bytes, offset: int) -> int:
    return struct.unpack_from("<I", data, offset)[0]


def validate(path: Path) -> dict[str, object]:
    data = path.read_bytes()
    checks: list[dict[str, object]] = []

    def check(name: str, condition: bool, observed: object) -> None:
        checks.append(
            {
                "name": name,
                "status": "PASS" if condition else "FAIL",
                "observed": observed,
            }
        )

    check("size", len(data) == DEX_MINIMAL_SIZE, len(data))
    check("magic", data[:8] == b"dex\n035\x00", data[:8].hex())

    if len(data) >= DEX_HEADER_SIZE:
        check("file_size", u32(data, 32) == len(data), u32(data, 32))
        check("header_size", u32(data, 36) == DEX_HEADER_SIZE, u32(data, 36))
        check("endian_tag", u32(data, 40) == DEX_ENDIAN_TAG, hex(u32(data, 40)))
        check("map_off", u32(data, 48) == DEX_HEADER_SIZE, u32(data, 48))
        check("data_size", u32(data, 104) == DEX_MINIMAL_SIZE - DEX_HEADER_SIZE, u32(data, 104))
        check("data_off", u32(data, 108) == DEX_HEADER_SIZE, u32(data, 108))

        expected_signature = hashlib.sha1(data[32:]).digest()
        check("sha1_signature", data[12:32] == expected_signature, data[12:32].hex())

        expected_adler = zlib.adler32(data[12:]) & 0xFFFFFFFF
        check("adler32_checksum", u32(data, 8) == expected_adler, hex(u32(data, 8)))

    if len(data) >= DEX_MINIMAL_SIZE:
        map_off = u32(data, 48)
        check("map_count", u32(data, map_off) == 2, u32(data, map_off))
        check(
            "map_header_entry",
            u16(data, map_off + 4) == DEX_TYPE_HEADER
            and u32(data, map_off + 8) == 1
            and u32(data, map_off + 12) == 0,
            {
                "type": hex(u16(data, map_off + 4)),
                "count": u32(data, map_off + 8),
                "offset": u32(data, map_off + 12),
            },
        )
        check(
            "map_list_entry",
            u16(data, map_off + 16) == DEX_TYPE_MAP_LIST
            and u32(data, map_off + 20) == 1
            and u32(data, map_off + 24) == map_off,
            {
                "type": hex(u16(data, map_off + 16)),
                "count": u32(data, map_off + 20),
                "offset": u32(data, map_off + 24),
            },
        )

    passed = bool(checks) and all(item["status"] == "PASS" for item in checks)
    return {
        "schema": "raf.apkc.dex-contract.v1",
        "artifact": str(path),
        "artifact_sha256": hashlib.sha256(data).hexdigest(),
        "semantic_scope": "STRUCTURAL_DEX_CONTAINER_NO_CLASSES",
        "status": "PASS" if passed else "FAIL",
        "claim_allowed": passed,
        "checks": checks,
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("dex", type=Path)
    parser.add_argument("--pretty", action="store_true")
    args = parser.parse_args()

    try:
        report = validate(args.dex)
    except (OSError, ValueError, struct.error) as exc:
        report = {
            "schema": "raf.apkc.dex-contract.v1",
            "artifact": str(args.dex),
            "status": "FAIL",
            "claim_allowed": False,
            "error": str(exc),
        }

    json.dump(report, sys.stdout, ensure_ascii=False, indent=2 if args.pretty else None)
    sys.stdout.write("\n")
    return 0 if report.get("status") == "PASS" else 1


if __name__ == "__main__":
    raise SystemExit(main())
