#!/usr/bin/env python3
"""Bounded class-definition/method-presence gate, not a complete DEX verifier.

Accept legacy little-endian DEX 035/037/038/039/040, including APK multidex.
Never execute the input. A string occurrence alone does not prove class inclusion.
Format source: https://source.android.com/docs/core/runtime/dex-format
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import struct
import zipfile
import zlib

CLASS = b"Lcom/termux/rafacodephi/lowlevel/AuthorialBitOps;"
EXPECTED = {b"read": 2, b"write": 3, b"count": 1, b"parity": 1}
LIMIT = 64 * 1024 * 1024


def require(ok, message):
    if not ok:
        raise ValueError(message)


def inspect(data):
    data = bytes(data)
    require(112 <= len(data) <= LIMIT, "DEX size")
    require(data[:8] in [b"dex\n" + v + b"\0" for v in
                         (b"035", b"037", b"038", b"039", b"040")], "DEX version")

    def u32(off):
        require(0 <= off <= len(data) - 4, "u32 bounds")
        return struct.unpack_from("<I", data, off)[0]

    require(u32(32) == len(data) and u32(36) == 112, "DEX header size")
    require(u32(40) == 0x12345678, "DEX endian")
    require(data[12:32] == hashlib.sha1(data[32:]).digest(), "DEX signature")
    require(u32(8) == zlib.adler32(data[12:]) & 0xFFFFFFFF, "DEX checksum")

    def table(header, width):
        size, off = u32(header), u32(header + 4)
        require((size == 0 and off == 0) or
                (off >= 112 and off % 4 == 0 and off + size * width <= len(data)),
                "table bounds")
        return size, off

    strings, so = table(56, 4)
    types, to = table(64, 4)
    protos, po = table(72, 12)
    methods, mo = table(88, 8)
    classes, co = table(96, 32)

    def leb(off):
        value = 0
        for n in range(5):
            require(off < len(data), "uleb bounds")
            b = data[off]
            off += 1
            require(n < 4 or b < 16, "uleb overflow")
            value |= (b & 127) << (7 * n)
            if b < 128:
                return value, off
        raise ValueError("uleb length")

    def string(index):
        require(index < strings, "string index")
        _, off = leb(u32(so + index * 4))
        end = data.find(b"\0", off, min(len(data), off + 65536))
        require(end >= off, "string bounds")
        return data[off:end]

    def typename(index):
        require(index < types, "type index")
        return string(u32(to + index * 4))

    matches = []
    for c in range(classes):
        off = co + c * 32
        index = u32(off)
        if typename(index) != CLASS:
            continue
        pos = u32(off + 24)
        require(pos >= 112, "class data missing")
        counts = []
        for _ in range(4):
            count, pos = leb(pos)
            require(count <= len(data), "class data count")
            counts.append(count)
        for _ in range(counts[0] + counts[1]):
            _, pos = leb(pos)
            _, pos = leb(pos)
        found = set()
        for count in counts[2:]:
            mid = 0
            for _ in range(count):
                delta, pos = leb(pos)
                flags, pos = leb(pos)
                code, pos = leb(pos)
                mid += delta
                require(mid < methods, "method index")
                owner, proto = struct.unpack_from("<HH", data, mo + mid * 8)
                require(owner == index and proto < protos, "method ownership")
                name = string(u32(mo + mid * 8 + 4))
                if name not in EXPECTED:
                    continue
                require(name not in found, "duplicate API method")
                require(flags & 9 == 9 and not flags & (0x100 | 0x400),
                        "API must be public static with DEX code, never native/abstract")
                require(code >= 112 and code % 4 == 0 and code + 16 <= len(data),
                        "code bounds")
                units = u32(code + 12)
                require(units > 0 and code + 16 + units * 2 <= len(data), "code size")
                p = po + proto * 12
                require(typename(u32(p + 4)) == b"I", "API return type")
                params = u32(p + 8)
                require(params >= 112 and params % 4 == 0, "parameter list")
                n = u32(params)
                require(n == EXPECTED[name] and params + 4 + n * 2 <= len(data),
                        "API arity")
                for k in range(n):
                    tid = struct.unpack_from("<H", data, params + 4 + 2 * k)[0]
                    require(typename(tid) == b"I", "API parameter type")
                found.add(name)
        require(found == set(EXPECTED), "API missing from class definition")
        matches.append(sorted(x.decode() for x in found))
    require(len(matches) <= 1, "duplicate class definitions")
    return len(matches)


def verify(path):
    if path.suffix == ".dex":
        require(path.stat().st_size <= LIMIT, "DEX too large")
        data = path.read_bytes()
        require(inspect(data) == 1, "class definition absent")
        digest = hashlib.sha256(data).hexdigest()
    else:
        found = 0
        with zipfile.ZipFile(path) as apk:
            entries = [e for e in apk.infolist() if re.fullmatch(r"classes(?:[2-9]|[1-9][0-9]+)?\.dex", e.filename)]
            require(entries and len(entries) <= 128, "DEX entries missing/oversized")
            require(len({e.filename for e in entries}) == len(entries), "duplicate DEX entries")
            require(sum(e.file_size for e in entries) <= LIMIT, "DEX input budget")
            for entry in entries:
                found += inspect(apk.read(entry))
        require(found == 1, "expected one class definition across multidex")
        h = hashlib.sha256()
        with path.open("rb") as stream:
            for chunk in iter(lambda: stream.read(65536), b""):
                h.update(chunk)
        digest = h.hexdigest()
    return {"artifact": path.name, "sha256": digest, "status": "PASS_SCOPED",
            "scope": "DEX_CLASS_DEFINITION_AND_API_PRESENCE", "claim_allowed": False,
            "device_runtime": "TOKEN_VAZIO_NOT_RUN", "performance": "TOKEN_VAZIO_PERFORMANCE_UNMEASURED"}


def artifact_paths(path):
    # Share the bootstrap gate's bounded AGP metadata discovery.
    if not path.exists() and path.parent.name == "apk" and path.name in ("debug", "release"):
        import os
        if __package__:
            from .verify_apk_bootstrap_payload import discover_packaged_apk_directory
        else:
            from verify_apk_bootstrap_payload import discover_packaged_apk_directory
        folder = discover_packaged_apk_directory(path, os.environ.get("TERMUX_APK_VERSION_TAG", ""))
        metadata = json.loads((folder / "output-metadata.json").read_text(encoding="utf-8"))
        names = [entry["outputFile"] for entry in metadata["elements"]]
        require(len(names) == len(set(names)), "duplicate APK metadata outputs")
        return [folder / name for name in sorted(names)]
    return sorted(path.glob("*.apk")) if path.is_dir() else [path]


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("artifacts", nargs="+", type=Path)
    args = parser.parse_args()
    paths = []
    for path in args.artifacts:
        paths.extend(artifact_paths(path))
    require(bool(paths), "no artifacts")
    print(json.dumps([verify(p) for p in paths], indent=2))
