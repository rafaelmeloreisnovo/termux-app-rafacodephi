"""Host-only negative tests for physical E2E reference receipt promotion."""
from __future__ import annotations

import importlib.util
import json
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location(
    "receipt_gate", ROOT / "scripts/validate_e2e_receipt.py")
gate = importlib.util.module_from_spec(spec)
spec.loader.exec_module(gate)


def fixture(receipt_id: str) -> dict:
    return {
        "receipt_id": receipt_id,
        "claim_allowed": False,
        "stages": {k: ("TOKEN_VAZIO" if k == "reproduction" else "PASS")
                   for k in gate.STAGES},
        "provenance": {
            "repository": gate.REPO,
            "git_commit": "a" * 40,
            "build_receipt_sha256": "b" * 64,
            "apk_sha256": "c" * 64,
            "bootstrap_source_repository": "rafaelmeloreisnovo/termux-packages",
            "bootstrap_source_commit": "d" * 40,
            "bootstrap_source_manifest_sha256": "e" * 64,
            "bootstrap_sha256": "f" * 64,
            "bootstrap_profile_sha256": "1" * 64,
        },
        "device": {"installed_apk_sha256": "c" * 64},
        "workload": {"exit_code": 0, "command": "test-command",
                    "stdout_sha256": "2" * 64},
    }


def structurally_valid_receipt() -> dict:
    receipt = fixture("0" * 64)
    receipt["schema"] = gate.SCHEMA
    receipt["generated_at"] = "2026-11-21T12:00:00-03:00"
    receipt["device"].update({
        "manufacturer": "fixture", "model": "fixture-arm32",
        "android_release": "10", "abi": "armeabi-v7a",
        "package": gate.PACKAGE, "prefix": gate.PREFIX,
        "installed_apk_path": "/data/app/fixture/base.apk",
    })
    receipt["receipt_id"] = gate.canonical_receipt_id(receipt)
    return receipt


class ReceiptInputValidationTests(unittest.TestCase):
    def test_valid_receipt_structure(self):
        self.assertEqual(gate.validate_structure(structurally_valid_receipt()), [])

    def test_root_array_is_blocked(self):
        self.assertIn("JSON object", "; ".join(gate.validate_structure([])))

    def test_boolean_exit_code_is_blocked(self):
        receipt = structurally_valid_receipt()
        receipt["workload"]["exit_code"] = False
        self.assertIn("boolean", "; ".join(gate.validate_structure(receipt)))

    def test_unexpected_key_is_blocked(self):
        receipt = structurally_valid_receipt()
        receipt["device"]["extra"] = "untrusted"
        self.assertIn("device keys", "; ".join(gate.validate_structure(receipt)))

    def test_single_receipt_cannot_self_promote(self):
        receipt = structurally_valid_receipt()
        receipt["stages"]["reproduction"] = "PASS"
        receipt["claim_allowed"] = True
        errors = "; ".join(gate.validate_structure(receipt))
        self.assertIn("single device receipt", errors)

    def test_naive_timestamp_is_blocked(self):
        receipt = structurally_valid_receipt()
        receipt["generated_at"] = "2026-11-21T12:00:00"
        self.assertIn("timezone offset", "; ".join(gate.validate_structure(receipt)))

    def test_duplicate_json_key_is_blocked(self):
        with tempfile.TemporaryDirectory() as td:
            file = Path(td) / "dup.json"
            file.write_text('{"schema":"a","schema":"b"}', encoding="utf-8")
            with self.assertRaises(gate.ReceiptError):
                gate.load(file)

    def test_unhashable_abi_is_blocked_cleanly(self):
        receipt = structurally_valid_receipt()
        receipt["device"]["abi"] = ["armeabi-v7a"]
        self.assertIn("device.abi", "; ".join(gate.validate_structure(receipt)))

    def test_unhashable_stage_is_blocked_cleanly(self):
        receipt = structurally_valid_receipt()
        receipt["stages"]["device"] = {"unexpected": "object"}
        self.assertIn("stages.device", "; ".join(gate.validate_structure(receipt)))

    def test_non_dict_result_does_not_crash(self):
        from io import StringIO
        from contextlib import redirect_stdout
        out = StringIO()
        with redirect_stdout(out):
            gate.emit_result(False, "receipt must be object", ["invalid"])
        self.assertIn('"gate": "BLOCKED"', out.getvalue())

    def test_oversized_receipt_is_blocked(self):
        with tempfile.TemporaryDirectory() as td:
            file = Path(td) / "oversized.json"
            file.write_bytes(b" " * (gate.MAX_RECEIPT_BYTES + 1))
            with self.assertRaises(gate.ReceiptError):
                gate.load(file)


class ReferenceGateTests(unittest.TestCase):
    def setUp(self):
        self.a = fixture("a" * 64)
        self.b = fixture("b" * 64)

    def test_matching_pair_can_pass_scoped_promotion(self):
        self.assertTrue(gate.promotion_state(self.b, self.a)[0])

    def test_nonzero_reference_exit_blocks(self):
        self.a["workload"]["exit_code"] = 127
        self.assertFalse(gate.promotion_state(self.b, self.a)[0])

    def test_reference_apk_mismatch_blocks(self):
        self.a["device"]["installed_apk_sha256"] = "e" * 64
        self.assertFalse(gate.promotion_state(self.b, self.a)[0])

    def test_reference_cannot_self_authorize(self):
        self.a["claim_allowed"] = True
        self.assertFalse(gate.promotion_state(self.b, self.a)[0])

    def test_identical_receipt_id_blocks(self):
        self.b["receipt_id"] = self.a["receipt_id"]
        self.assertFalse(gate.promotion_state(self.b, self.a)[0])


if __name__ == "__main__":
    unittest.main()
