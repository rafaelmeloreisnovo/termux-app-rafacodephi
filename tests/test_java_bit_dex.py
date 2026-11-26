#!/usr/bin/env python3
"""Adversarial mutations of a real D8 output; never executes mutated input."""
import importlib.util
from pathlib import Path
import hashlib
import struct
import sys
import tempfile
import unittest
import zipfile
import zlib

spec = importlib.util.spec_from_file_location("dexgate", Path(__file__).resolve().parents[1] / "scripts/verify_java_bit_dex.py")
gate = importlib.util.module_from_spec(spec)
spec.loader.exec_module(gate)
DATA = Path(sys.argv.pop(1)).read_bytes()


def seal(data):
    data[12:32] = hashlib.sha1(data[32:]).digest()
    struct.pack_into("<I", data, 8, zlib.adler32(data[12:]) & 0xFFFFFFFF)
    return data


class DexTests(unittest.TestCase):
    def test_real_d8_class(self):
        self.assertEqual(gate.inspect(DATA), 1)

    def test_truncated(self):
        for end in (0, 8, 111, len(DATA)-1):
            with self.subTest(end=end), self.assertRaises(ValueError):
                gate.inspect(DATA[:end])

    def test_bad_header_fields(self):
        for off, value in ((32, 1), (36, 1), (40, 0), (60, 0xFFFFFFFC),
                           (68, 0xFFFFFFFC), (76, 0xFFFFFFFC), (92, 0xFFFFFFFC),
                           (100, 0xFFFFFFFC)):
            data = bytearray(DATA)
            struct.pack_into("<I", data, off, value)
            with self.subTest(off=off), self.assertRaises(ValueError):
                gate.inspect(seal(data))

    def test_unsupported_versions(self):
        for version in (b"036", b"041", b"999"):
            data = bytearray(DATA)
            data[4:7] = version
            with self.subTest(version=version), self.assertRaises(ValueError):
                gate.inspect(seal(data))

    def test_checksum_and_signature(self):
        for off in (8, 12, len(DATA)-1):
            data = bytearray(DATA)
            data[off] ^= 1
            with self.subTest(off=off), self.assertRaises(ValueError):
                gate.inspect(data)

    def test_string_only_is_not_class(self):
        data = bytearray(DATA)
        struct.pack_into("<II", data, 96, 0, 0)
        self.assertEqual(gate.inspect(seal(data)), 0)

    def test_missing_api(self):
        data = bytearray(DATA)
        pos = data.index(b"parity\0")
        data[pos:pos+6] = b"rarity"
        with self.assertRaises(ValueError):
            gate.inspect(seal(data))

    def test_oversized_uleb(self):
        data = bytearray(DATA)
        pos = data.index(gate.CLASS) - 1
        data[pos:pos+5] = b"\xff" * 5
        with self.assertRaises(ValueError):
            gate.inspect(seal(data))

    def test_apk_multidex_and_duplicate_class(self):
        with tempfile.TemporaryDirectory() as tmp:
            p = Path(tmp)/"sample.apk"
            absent = bytearray(DATA)
            struct.pack_into("<II", absent, 96, 0, 0)
            with zipfile.ZipFile(p, "w") as z:
                z.writestr("classes.dex", seal(absent))
                z.writestr("classes2.dex", DATA)
            self.assertEqual(gate.verify(p)["status"], "PASS_SCOPED")
            with zipfile.ZipFile(p, "w") as z:
                z.writestr("classes.dex", DATA)
                z.writestr("classes2.dex", DATA)
            with self.assertRaises(ValueError):
                gate.verify(p)


if __name__ == "__main__":
    unittest.main()
