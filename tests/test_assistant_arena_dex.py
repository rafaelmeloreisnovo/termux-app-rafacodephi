#!/usr/bin/env python3
"""Adversarial DEX mutations: accepts only one fully defined arena class."""
import hashlib
import importlib.util
from pathlib import Path
import struct
import sys
import unittest
import zlib

spec = importlib.util.spec_from_file_location("arena_dex_gate", Path(__file__).resolve().parents[1] /
                                              "scripts/verify_assistant_arena_dex.py")
gate = importlib.util.module_from_spec(spec)
spec.loader.exec_module(gate)
DATA = Path(sys.argv.pop(1)).read_bytes()


def reseal(data):
    data[12:32] = hashlib.sha1(data[32:]).digest()
    struct.pack_into("<I", data, 8, zlib.adler32(data[12:]) & 0xFFFFFFFF)
    return data


class ArenaDexAdversarial(unittest.TestCase):
    def test_positive(self):
        self.assertEqual(len(gate.inspect(DATA)["verified_methods"]), 11)

    def test_truncation(self):
        for n in (0, 8, 111, len(DATA) - 1):
            with self.subTest(n=n), self.assertRaises(ValueError):
                gate.inspect(DATA[:n])

    def test_integrity(self):
        for pos in (8, 12, len(DATA)-1):
            d = bytearray(DATA)
            d[pos] ^= 1
            with self.subTest(pos=pos), self.assertRaises(ValueError):
                gate.inspect(d)

    def test_wrong_version(self):
        d = bytearray(DATA)
        d[4:7] = b"041"
        with self.assertRaises(ValueError):
            gate.inspect(reseal(d))

    def test_missing_class_definition(self):
        d = bytearray(DATA)
        struct.pack_into("<II", d, 96, 0, 0)
        with self.assertRaises(ValueError):
            gate.inspect(reseal(d))

    def test_missing_public_method(self):
        d = bytearray(DATA)
        pos = d.index(b"heartbeat\0")
        d[pos] = ord("x")
        with self.assertRaises(ValueError):
            gate.inspect(reseal(d))

    def test_corrupt_data_table(self):
        d = bytearray(DATA)
        struct.pack_into("<I", d, 100, 0xFFFFFFFC)
        with self.assertRaises(ValueError):
            gate.inspect(reseal(d))


if __name__ == "__main__":
    unittest.main()
