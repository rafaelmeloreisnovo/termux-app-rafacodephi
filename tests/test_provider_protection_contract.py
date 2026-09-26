from __future__ import annotations

import importlib.util
from pathlib import Path
import unittest


class ProviderProtectionContractTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.root = Path(__file__).resolve().parents[1]
        module_path = cls.root / "scripts/ci/provider_protection_contract.py"
        spec = importlib.util.spec_from_file_location("provider_protection_contract", module_path)
        assert spec and spec.loader
        cls.mod = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(cls.mod)

        cls.target_path = cls.root / "governance/provider/PROVIDER_RULESET_TARGET_20260831.v1.json"
        cls.contract = cls.mod.load_target(cls.target_path)

    def _live_ruleset(self, *, include_status: bool = True, code_owner: bool = False):
        rules = [
            {"type": "deletion"},
            {"type": "non_fast_forward"},
            {"type": "required_linear_history"},
            {"type": "required_signatures"},
            {"type": "code_quality"},
            {
                "type": "pull_request",
                "parameters": {
                    "required_approving_review_count": 0,
                    "require_code_owner_review": code_owner,
                    "required_review_thread_resolution": True,
                    "dismiss_stale_reviews_on_push": False,
                    "require_last_push_approval": False,
                    "allowed_merge_methods": ["squash", "rebase"],
                },
            },
        ]
        if include_status:
            rules.append(
                {
                    "type": "required_status_checks",
                    "parameters": {
                        "strict_required_status_checks_policy": True,
                        "required_status_checks": [
                            {"context": "provider-protection", "integration_id": 15368}
                        ],
                    },
                }
            )
        return [
            {
                "id": 21908888,
                "name": "enterprise-default-branch",
                "enforcement": "active",
                "conditions": {"ref_name": {"include": ["~DEFAULT_BRANCH"], "exclude": []}},
                "rules": rules,
                "bypass_actors": [],
            }
        ]

    def _evaluate(self, live):
        return self.mod.evaluate(
            self.contract,
            live,
            target_path=str(self.target_path.relative_to(self.root)),
            target_sha256=self.mod.file_sha256(self.target_path),
            repository="rafaelmeloreisnovo/termux-app-rafacodephi",
            default_branch="master",
        )

    def test_target_schema_is_canonical(self) -> None:
        self.assertEqual(
            self.contract["schema"],
            "rafaelia.provider_ruleset_target/v1",
        )
        self.assertFalse(self.contract["claim_allowed"])

    def test_full_target_match_passes(self) -> None:
        receipt = self._evaluate(self._live_ruleset())
        self.assertEqual(receipt["gate"], "PASS")
        self.assertEqual(receipt["failures"], [])
        self.assertTrue(receipt["checks"]["required_rule_types"]["pass"])
        self.assertTrue(receipt["checks"]["pull_request"]["pass"])
        self.assertTrue(receipt["checks"]["required_status_checks"]["pass"])
        self.assertFalse(receipt["claim_allowed"])
        self.assertEqual(
            receipt["remediation"]["apply_state"],
            "TOKEN_VAZIO_NOT_APPLIED_BY_THIS_EVALUATOR",
        )

    def test_missing_required_status_checks_fails_closed(self) -> None:
        receipt = self._evaluate(self._live_ruleset(include_status=False))
        self.assertEqual(receipt["gate"], "FAIL")
        codes = {item["code"] for item in receipt["failures"]}
        self.assertIn("MISSING_RULE_TYPES", codes)
        self.assertIn("REQUIRED_STATUS_CHECKS_MISMATCH", codes)
        operations = {item["kind"] for item in receipt["remediation"]["operations"]}
        self.assertIn("ENSURE_REQUIRED_STATUS_CHECKS", operations)

    def test_pull_request_policy_mismatch_fails_closed(self) -> None:
        receipt = self._evaluate(self._live_ruleset(code_owner=True))
        self.assertEqual(receipt["gate"], "FAIL")
        codes = {item["code"] for item in receipt["failures"]}
        self.assertIn("PULL_REQUEST_POLICY_MISMATCH", codes)
        operations = {item["kind"] for item in receipt["remediation"]["operations"]}
        self.assertIn("ALIGN_PULL_REQUEST_POLICY", operations)

    def test_unknown_always_bypass_is_not_silently_promoted(self) -> None:
        live = self._live_ruleset()
        live[0]["bypass_actors"] = [
            {"actor_type": "Integration", "actor_id": 999999, "bypass_mode": "always"}
        ]
        receipt = self._evaluate(live)
        self.assertEqual(
            receipt["live_observation"]["bypass_identity_state"],
            "TOKEN_VAZIO_PENDING_IDENTITY_AND_JUSTIFICATION",
        )
        self.assertFalse(receipt["claim_allowed"])

    def test_target_and_live_are_hash_addressed_separately(self) -> None:
        receipt = self._evaluate(self._live_ruleset())
        self.assertRegex(receipt["target"]["sha256"], r"^[0-9a-f]{64}$")
        self.assertRegex(
            receipt["live_observation"]["digest_sha256"],
            r"^[0-9a-f]{64}$",
        )
        self.assertNotEqual(
            receipt["target"]["sha256"],
            receipt["live_observation"]["digest_sha256"],
        )


if __name__ == "__main__":
    unittest.main()
