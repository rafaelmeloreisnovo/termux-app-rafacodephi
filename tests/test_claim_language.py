from __future__ import annotations

import json
import tempfile
import unittest
from pathlib import Path

from scripts.validate_claim_language import validate


class ClaimLanguageTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.root = Path(self.temp.name)
        self.readme = self.root / "README.md"
        self.debt = self.root / "debt.json"
        self.base = "# X\nfully compatible\nwill NOT crash\nproduction-ready\nZero Collisions\n"
        self.cfg = {
            "schema": "rafacodephi.claim-boundary-debt/v1",
            "claim_allowed": False,
            "items": [
                {"rule_id": "fully_compatible", "max_count": 1, "claim_allowed": False},
                {"rule_id": "no_crash_guarantee", "max_count": 1, "claim_allowed": False},
                {"rule_id": "production_ready", "max_count": 1, "claim_allowed": False},
                {"rule_id": "zero_collisions", "max_count": 1, "claim_allowed": False}
            ]
        }
        self.write(self.base, self.cfg)

    def tearDown(self):
        self.temp.cleanup()

    def write(self, text, cfg):
        self.readme.write_text(text, encoding="utf-8")
        self.debt.write_text(json.dumps(cfg), encoding="utf-8")

    def test_known_debt_is_bounded_not_promoted(self):
        report = validate(self.readme, self.debt)
        self.assertEqual(report["status"], "PASS_WITH_KNOWN_DEBT")
        self.assertFalse(report["claim_allowed"])

    def test_duplicate_known_debt_fails(self):
        self.readme.write_text(self.base + "\nfully compatible\n", encoding="utf-8")
        self.assertEqual(validate(self.readme, self.debt)["status"], "FAIL")

    def test_new_unallowlisted_absolute_claim_fails(self):
        self.readme.write_text(self.base + "\nguaranteed universally compatible\n", encoding="utf-8")
        self.assertEqual(validate(self.readme, self.debt)["status"], "FAIL")

    def test_debt_reduction_passes_with_warning(self):
        self.readme.write_text("# X\nwill NOT crash\nproduction-ready\nZero Collisions\n", encoding="utf-8")
        report = validate(self.readme, self.debt)
        self.assertNotEqual(report["status"], "FAIL")
        self.assertTrue(report["warnings"])

    def test_claim_allowed_true_in_debt_fails(self):
        self.cfg["items"][0]["claim_allowed"] = True
        self.write(self.base, self.cfg)
        self.assertEqual(validate(self.readme, self.debt)["status"], "FAIL")

    def test_unknown_rule_fails(self):
        self.cfg["items"].append({"rule_id": "made_up", "max_count": 1, "claim_allowed": False})
        self.write(self.base, self.cfg)
        self.assertEqual(validate(self.readme, self.debt)["status"], "FAIL")


if __name__ == "__main__":
    unittest.main()
