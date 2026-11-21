"""Host-only negative tests for physical E2E reference receipt promotion."""
from __future__ import annotations

import importlib.util
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
