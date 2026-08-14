import hashlib
import importlib.util
import json
import struct
import sys
import tempfile
import unittest
from pathlib import Path

MODULE_PATH = Path(__file__).resolve().parents[1] / "scripts" / "verify_apk_evidence.py"
spec = importlib.util.spec_from_file_location("verify_apk_evidence", MODULE_PATH)
module = importlib.util.module_from_spec(spec)
sys.modules[spec.name] = module
spec.loader.exec_module(module)


def _utf8_len(value):
    if value < 0x80:
        return bytes([value])
    return bytes([0x80 | (value >> 8), value & 0xFF])


def _string_pool(strings):
    payload = b""
    offsets = []
    for value in strings:
        raw = value.encode("utf-8")
        utf16_chars = len(value.encode("utf-16le")) // 2
        offsets.append(len(payload))
        payload += _utf8_len(utf16_chars) + _utf8_len(len(raw)) + raw + b"\x00"
    header_size = 28
    offset_table = b"".join(struct.pack("<I", offset) for offset in offsets)
    strings_start = header_size + len(offset_table)
    body = (
        struct.pack(
            "<IIIII",
            len(strings),
            0,
            module.UTF8_FLAG,
            strings_start,
            0,
        )
        + offset_table
        + payload
    )
    return struct.pack(
        "<HHI", module.RES_STRING_POOL_TYPE, header_size, 8 + len(body)
    ) + body


def _manifest_start(strings):
    index = {value: i for i, value in enumerate(strings)}
    attrs = []

    def add_attr(name, raw=None, data_type=module.TYPE_STRING, data=0):
        raw_index = module.NO_INDEX if raw is None else index[raw]
        if raw is not None and data_type == module.TYPE_STRING:
            data = index[raw]
        attrs.append(
            struct.pack(
                "<IIIHBBI",
                module.NO_INDEX,
                index[name],
                raw_index,
                8,
                0,
                data_type,
                data,
            )
        )

    add_attr("package", "com.example.app")
    add_attr("versionName", "1.2.3")
    add_attr("versionCode", None, 0x10, 42)
    attr_ext = struct.pack(
        "<IIHHHHHH",
        module.NO_INDEX,
        index["manifest"],
        20,
        20,
        len(attrs),
        0,
        0,
        0,
    )
    node = struct.pack("<II", 1, module.NO_INDEX)
    body = node + attr_ext + b"".join(attrs)
    return struct.pack(
        "<HHI", module.RES_XML_START_ELEMENT_TYPE, 16, 8 + len(body)
    ) + body


def _axml():
    strings = [
        "manifest",
        "package",
        "com.example.app",
        "versionName",
        "1.2.3",
        "versionCode",
    ]
    string_pool = _string_pool(strings)
    start = _manifest_start(strings)
    size = 8 + len(string_pool) + len(start)
    return struct.pack("<HHI", 0x0003, 8, size) + string_pool + start


def _write_receipt(path, source_commit, artifact_sha256, clean_before=True, clean_after=True):
    data = {
        "schema": "rafaelia.apk-build-provenance.v1",
        "source_commit": source_commit,
        "source_tree_clean_before_build": clean_before,
        "source_tree_clean_after_build": clean_after,
        "artifacts": [{"sha256": artifact_sha256}],
        "claim_allowed": False,
    }
    path.write_text(json.dumps(data, sort_keys=True) + "\n", encoding="utf-8")
    return hashlib.sha256(path.read_bytes()).hexdigest()


class ApkEvidenceTests(unittest.TestCase):
    def test_parse_manifest_axml_extracts_identity(self):
        result = module.parse_manifest_axml(_axml())
        self.assertEqual(result["package"], "com.example.app")
        self.assertEqual(result["version_name"], "1.2.3")
        self.assertEqual(result["version_code"], 42)

    def test_parse_manifest_axml_rejects_invalid_root(self):
        with self.assertRaises(Exception):
            module.parse_manifest_axml(b"12345678")

    def test_build_provenance_positive_link(self):
        artifact_sha = "a" * 64
        source_commit = "b" * 40
        with tempfile.TemporaryDirectory() as directory:
            receipt = Path(directory) / "BUILD_PROVENANCE.json"
            receipt_sha = _write_receipt(receipt, source_commit, artifact_sha)
            contract = {
                "build_provenance": {
                    "source_commit": source_commit,
                    "build_receipt_sha256": receipt_sha,
                }
            }
            result, passed, required = module.verify_build_provenance(
                contract, receipt, artifact_sha
            )
            self.assertTrue(required)
            self.assertTrue(passed)
            self.assertEqual(result["status"], "PASS")
            self.assertTrue(result["artifact_listed"])

    def test_build_provenance_rejects_receipt_hash_mismatch(self):
        artifact_sha = "c" * 64
        source_commit = "d" * 40
        with tempfile.TemporaryDirectory() as directory:
            receipt = Path(directory) / "BUILD_PROVENANCE.json"
            _write_receipt(receipt, source_commit, artifact_sha)
            contract = {
                "build_provenance": {
                    "source_commit": source_commit,
                    "build_receipt_sha256": "0" * 64,
                }
            }
            result, passed, required = module.verify_build_provenance(
                contract, receipt, artifact_sha
            )
            self.assertTrue(required)
            self.assertFalse(passed)
            self.assertEqual(result["status"], "FAIL")

    def test_build_provenance_rejects_unlisted_artifact(self):
        source_commit = "e" * 40
        with tempfile.TemporaryDirectory() as directory:
            receipt = Path(directory) / "BUILD_PROVENANCE.json"
            receipt_sha = _write_receipt(receipt, source_commit, "1" * 64)
            contract = {
                "build_provenance": {
                    "source_commit": source_commit,
                    "build_receipt_sha256": receipt_sha,
                }
            }
            result, passed, required = module.verify_build_provenance(
                contract, receipt, "2" * 64
            )
            self.assertTrue(required)
            self.assertFalse(passed)
            self.assertEqual(result["status"], "FAIL")
            self.assertFalse(result["artifact_listed"])

    def test_build_provenance_rejects_dirty_build(self):
        artifact_sha = "3" * 64
        source_commit = "f" * 40
        with tempfile.TemporaryDirectory() as directory:
            receipt = Path(directory) / "BUILD_PROVENANCE.json"
            receipt_sha = _write_receipt(
                receipt, source_commit, artifact_sha, clean_before=True, clean_after=False
            )
            contract = {
                "build_provenance": {
                    "source_commit": source_commit,
                    "build_receipt_sha256": receipt_sha,
                }
            }
            result, passed, required = module.verify_build_provenance(
                contract, receipt, artifact_sha
            )
            self.assertTrue(required)
            self.assertFalse(passed)
            self.assertEqual(result["status"], "FAIL")


if __name__ == "__main__":
    unittest.main(verbosity=2)
