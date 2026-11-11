from __future__ import annotations

import copy
import importlib.util
import json
from pathlib import Path
import unittest


class MasterRulesetGuardTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        root = Path(__file__).resolve().parents[1]
        path = root / "scripts/ci/master_ruleset_gate.py"
        spec = importlib.util.spec_from_file_location("master_ruleset_gate", path)
        assert spec and spec.loader
        cls.mod = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(cls.mod)
        cls.payload = json.loads((root / cls.mod.POLICY).read_text(encoding="utf-8"))

    def test_validated_payload_is_no_bypass_and_scoped(self):
        self.mod.validate_policy(self.payload)
        self.assertEqual(self.payload["bypass_actors"], [])
        self.assertEqual(self.payload["conditions"]["ref_name"]["include"], ["~DEFAULT_BRANCH"])

    def test_no_bypass_expansion_permitted(self):
        bad = copy.deepcopy(self.payload)
        bad["bypass_actors"] = [{"actor_id": 5, "actor_type": "RepositoryRole", "bypass_mode": "always"}]
        with self.assertRaises(ValueError):
            self.mod.validate_policy(bad)

    def test_missing_check_fails_closed(self):
        bad = copy.deepcopy(self.payload)
        bad["rules"][-1]["parameters"]["required_status_checks"] = []
        with self.assertRaises(ValueError):
            self.mod.validate_policy(bad)

    def test_wrong_branch_fails_closed(self):
        bad = copy.deepcopy(self.payload)
        bad["conditions"]["ref_name"]["include"] = ["~ALL"]
        with self.assertRaises(ValueError):
            self.mod.validate_policy(bad)

    def test_less_strict_status_fails_closed(self):
        bad = copy.deepcopy(self.payload)
        bad["rules"][-1]["parameters"]["strict_required_status_checks_policy"] = False
        with self.assertRaises(ValueError):
            self.mod.validate_policy(bad)

    def test_live_readback_requires_all_rules(self):
        self.assertTrue(self.mod.matches_live(self.payload, copy.deepcopy(self.payload)))
        changed = copy.deepcopy(self.payload)
        changed["rules"] = changed["rules"][:-1]
        self.assertFalse(self.mod.matches_live(self.payload, changed))

    def test_wrong_rule_method_fails(self):
        bad = copy.deepcopy(self.payload)
        bad["rules"][-2]["parameters"]["allowed_merge_methods"] = ["merge"]
        with self.assertRaises(ValueError):
            self.mod.validate_policy(bad)


if __name__ == "__main__":
    unittest.main()
