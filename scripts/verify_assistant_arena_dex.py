#!/usr/bin/env python3
"""Fail-closed, bounded DEX class/API structural inspector for the assistant arena.

Build-time only. Does not execute DEX, prove ART operation or scan entire APK.
"""
import hashlib
import json
from pathlib import Path
import struct
import sys
import zlib

CLASS = b"Lcom/termux/rafacodephi/assistant/RafAssistantArenaV1;"
API = {
    b"begin": (b"I", ()),
    b"read": (b"I", (b"I",)),
    b"write": (b"I", (b"I", b"I")),
    b"commit": (b"I", ()),
    b"rollback": (b"I", ()),
    b"armWatchdog": (b"I", (b"J", b"J")),
    b"heartbeat": (b"I", (b"J",)),
    b"tripSafeMode": (b"V", ()),
    b"isSafeMode": (b"Z", ()),
    b"isInTransaction": (b"Z", ()),
    b"changedPages": (b"I", ()),
}
MAX_BYTES = 32 * 1024 * 1024


def require(ok, reason):
    if not ok:
        raise ValueError(reason)


def inspect(data):
    data = bytes(data)
    require(112 <= len(data) <= MAX_BYTES, "DEX_SIZE")
    require(data[:8] in [b"dex\n" + v + b"\0" for v in
                            (b"035", b"037", b"038", b"039", b"040")], "DEX_VERSION")

    def u32(pos):
        require(0 <= pos <= len(data) - 4, "U32_BOUNDS")
        return struct.unpack_from("<I", data, pos)[0]

    require(u32(32) == len(data) and u32(36) == 112, "DEX_HEADER_SIZE")
    require(u32(40) == 0x12345678, "DEX_ENDIAN")
    require(data[12:32] == hashlib.sha1(data[32:]).digest(), "DEX_SHA1")
    require(u32(8) == zlib.adler32(data[12:]) & 0xFFFFFFFF, "DEX_ADLER32")

    def table(off, stride):
        n, first = u32(off), u32(off + 4)
        require((n == 0 and first == 0) or
                (first >= 112 and first % 4 == 0 and first + n * stride <= len(data)),
                "DEX_TABLE_BOUNDS")
        return n, first

    ns, strings = table(56, 4)
    nt, types = table(64, 4)
    np, protos = table(72, 12)
    nm, methods = table(88, 8)
    nc, classes = table(96, 32)

    def leb(pos):
        v = 0
        for k in range(5):
            require(pos < len(data), "ULEB_BOUNDS")
            b = data[pos]
            pos += 1
            require(k != 4 or b < 16, "ULEB_OVERFLOW")
            v |= (b & 127) << (k * 7)
            if b < 128:
                return v, pos
        raise ValueError("ULEB_LENGTH")

    def s(index):
        require(index < ns, "STRING_INDEX")
        _, pos = leb(u32(strings + index * 4))
        end = data.find(b"\0", pos, min(len(data), pos + 2048))
        require(end >= 0, "STRING_TERMINATOR")
        return data[pos:end]

    def ty(index):
        require(index < nt, "TYPE_INDEX")
        return s(u32(types + index * 4))

    def signature(proto):
        require(proto < np, "PROTO_INDEX")
        pos = protos + proto * 12
        ret = ty(u32(pos + 4))
        param_off = u32(pos + 8)
        if not param_off:
            return ret, ()
        require(param_off >= 112 and param_off % 4 == 0, "PARAM_OFFSET")
        count = u32(param_off)
        require(count <= 16 and param_off + 4 + count * 2 <= len(data), "PARAM_BOUNDS")
        return ret, tuple(ty(struct.unpack_from("<H", data, param_off + 4 + 2 * i)[0])
                          for i in range(count))

    matched_classes = 0
    matched_methods = set()
    for ci in range(nc):
        pos = classes + 32 * ci
        class_index = u32(pos)
        if ty(class_index) != CLASS:
            continue
        matched_classes += 1
        cur = u32(pos + 24)
        require(cur >= 112 and cur < len(data), "CLASS_DATA_MISSING")
        counters = []
        for _ in range(4):
            count, cur = leb(cur)
            require(count <= len(data), "CLASS_COUNT_BOUNDS")
            counters.append(count)
        for _ in range(counters[0] + counters[1]):
            _, cur = leb(cur)
            _, cur = leb(cur)
        for count in counters[2:]:
            method_index = 0
            for _ in range(count):
                delta, cur = leb(cur)
                flags, cur = leb(cur)
                code, cur = leb(cur)
                method_index += delta
                require(method_index < nm, "METHOD_INDEX")
                owner, proto = struct.unpack_from("<HH", data, methods + method_index * 8)
                require(owner == class_index, "METHOD_OWNER")
                name = s(u32(methods + method_index * 8 + 4))
                if name not in API:
                    continue
                require(name not in matched_methods, "DUPLICATE_METHOD")
                require(flags & 1 and not flags & (8 | 0x100 | 0x400), "API_FLAGS")
                require(signature(proto) == API[name], "API_SIGNATURE")
                require(code >= 112 and code % 4 == 0 and code + 16 <= len(data),
                        "API_CODE_OFFSET")
                units = u32(code + 12)
                require(units > 0 and code + 16 + 2 * units <= len(data), "API_CODE_SIZE")
                matched_methods.add(name)
    require(matched_classes == 1, "ARENA_CLASS_COUNT")
    require(matched_methods == set(API), "ARENA_API_INCOMPLETE")
    return {"class_definitions": matched_classes,
            "verified_methods": sorted(x.decode("ascii") for x in matched_methods)}


def verify(path):
    data = path.read_bytes()
    result = inspect(data)
    return {"status": "PASS_SCOPED", "scope": "DEX_ARENA_CLASS_AND_API_CODE",
            "artifact": path.name, "sha256": hashlib.sha256(data).hexdigest(),
            "verified_methods": result["verified_methods"],
            "android_runtime": "TOKEN_VAZIO_NOT_RUN", "claim_allowed": False}


if __name__ == "__main__":
    if len(sys.argv) != 2:
        raise SystemExit("usage: verify_assistant_arena_dex.py classes.dex")
    print(json.dumps(verify(Path(sys.argv[1])), sort_keys=True, indent=2))
