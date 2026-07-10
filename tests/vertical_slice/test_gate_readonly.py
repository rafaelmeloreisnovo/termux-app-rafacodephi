"""
tests/vertical_slice/test_gate_readonly.py

Tests the governance gate logic:
- Capabilities outside the allowlist are BLOCKED.
- Free-text → shell is BLOCKED.
- Only git.read and git.diff are allowed in v1.
- Critical risk is BLOCKED.
- human_review halts execution.
- Only execution_gate=allow proceeds.
"""
import json
import unittest
from pathlib import Path

REPO_ROOT = Path(__file__).parent.parent.parent
CAPS_PATH = REPO_ROOT / "internal" / "governance" / "capabilities.json"
POLICY_PATH = REPO_ROOT / "internal" / "governance" / "policy.json"

# ─────────────────────────────────────────────────────────────
# Gate implementation (mirrors run_readonly_flow.sh logic)
# ─────────────────────────────────────────────────────────────

class GateDecision:
    ALLOW = "allow"
    BLOCKED = "blocked"
    HUMAN_REVIEW = "human_review"
    SANDBOX_ONLY = "sandbox_only"


V1_ALLOWED_CAPS = {"git.read", "git.diff"}


def apply_gate(intent: dict, capabilities_doc: dict) -> tuple[str, str]:
    """
    Returns (decision, reason).
    decision: one of GateDecision.*
    """
    allowed_ids = {c["id"] for c in capabilities_doc["capabilities"]}

    if intent.get("execution_gate") == "blocked":
        return GateDecision.BLOCKED, "execution_gate=blocked"

    if intent.get("risk") == "critical":
        return GateDecision.BLOCKED, "risk=critical"

    if intent.get("execution_gate") == "human_review" or intent.get("risk") == "high":
        return GateDecision.HUMAN_REVIEW, "human_review required"

    requested = intent.get("requested_capabilities", [])
    unknown = [c for c in requested if c not in allowed_ids]
    if unknown:
        return GateDecision.BLOCKED, f"unknown capabilities: {unknown}"

    non_v1 = [c for c in requested if c not in V1_ALLOWED_CAPS]
    if non_v1:
        return GateDecision.BLOCKED, f"v1 does not allow: {non_v1}"

    if intent.get("execution_gate") != "allow":
        return GateDecision.HUMAN_REVIEW, f"execution_gate={intent.get('execution_gate')}"

    return GateDecision.ALLOW, "ok"


# ─────────────────────────────────────────────────────────────
# Helpers
# ─────────────────────────────────────────────────────────────

def _valid_intent(**overrides):
    base = {
        "schema": "rafaelia.intent.v1",
        "intent_id": "gate-test-0001",
        "action": "git.status",
        "target": {"repo_path": "/tmp/testrepo"},
        "inputs": [],
        "constraints": [{"key": "read_only", "value": True}],
        "evidence_refs": [],
        "requested_capabilities": ["git.read"],
        "risk": "low",
        "execution_gate": "allow",
    }
    base.update(overrides)
    return base


# ─────────────────────────────────────────────────────────────
# Tests
# ─────────────────────────────────────────────────────────────

class TestGovernanceFiles(unittest.TestCase):
    def test_capabilities_file_exists(self):
        self.assertTrue(CAPS_PATH.exists(), f"Not found: {CAPS_PATH}")

    def test_capabilities_file_is_valid_json(self):
        with open(CAPS_PATH) as f:
            doc = json.load(f)
        self.assertIsInstance(doc, dict)

    def test_capabilities_has_allowlist(self):
        with open(CAPS_PATH) as f:
            doc = json.load(f)
        self.assertIn("capabilities", doc)
        self.assertIsInstance(doc["capabilities"], list)
        self.assertGreater(len(doc["capabilities"]), 0)

    def test_capabilities_default_policy_is_blocked(self):
        with open(CAPS_PATH) as f:
            doc = json.load(f)
        self.assertEqual(doc.get("default_policy"), "blocked")

    def test_capabilities_free_text_to_shell_blocked(self):
        with open(CAPS_PATH) as f:
            doc = json.load(f)
        self.assertEqual(doc.get("free_text_to_shell"), "blocked")

    def test_policy_file_exists(self):
        self.assertTrue(POLICY_PATH.exists(), f"Not found: {POLICY_PATH}")

    def test_policy_file_is_valid_json(self):
        with open(POLICY_PATH) as f:
            doc = json.load(f)
        self.assertIsInstance(doc, dict)

    def test_policy_has_rules(self):
        with open(POLICY_PATH) as f:
            doc = json.load(f)
        self.assertIn("rules", doc)
        self.assertGreater(len(doc["rules"]), 0)


class TestGateAllowCases(unittest.TestCase):
    def setUp(self):
        with open(CAPS_PATH) as f:
            self.caps = json.load(f)

    def test_git_read_low_risk_allow(self):
        intent = _valid_intent(requested_capabilities=["git.read"], risk="low")
        decision, reason = apply_gate(intent, self.caps)
        self.assertEqual(decision, GateDecision.ALLOW, reason)

    def test_git_diff_low_risk_allow(self):
        intent = _valid_intent(requested_capabilities=["git.diff"], risk="low")
        decision, reason = apply_gate(intent, self.caps)
        self.assertEqual(decision, GateDecision.ALLOW, reason)

    def test_both_v1_caps_allow(self):
        intent = _valid_intent(requested_capabilities=["git.read", "git.diff"], risk="low")
        decision, reason = apply_gate(intent, self.caps)
        self.assertEqual(decision, GateDecision.ALLOW, reason)

    def test_medium_risk_allow(self):
        intent = _valid_intent(risk="medium")
        decision, reason = apply_gate(intent, self.caps)
        self.assertEqual(decision, GateDecision.ALLOW, reason)


class TestGateBlockCases(unittest.TestCase):
    def setUp(self):
        with open(CAPS_PATH) as f:
            self.caps = json.load(f)

    def test_execution_gate_blocked(self):
        intent = _valid_intent(execution_gate="blocked")
        decision, _ = apply_gate(intent, self.caps)
        self.assertEqual(decision, GateDecision.BLOCKED)

    def test_critical_risk_blocked(self):
        intent = _valid_intent(risk="critical")
        decision, _ = apply_gate(intent, self.caps)
        self.assertEqual(decision, GateDecision.BLOCKED)

    def test_unknown_capability_blocked(self):
        intent = _valid_intent(requested_capabilities=["git.write"])
        decision, reason = apply_gate(intent, self.caps)
        self.assertEqual(decision, GateDecision.BLOCKED, reason)

    def test_filesystem_read_blocked_in_v1(self):
        intent = _valid_intent(requested_capabilities=["filesystem.read"])
        decision, reason = apply_gate(intent, self.caps)
        self.assertEqual(decision, GateDecision.BLOCKED, reason)

    def test_termux_command_safe_blocked_in_v1(self):
        intent = _valid_intent(requested_capabilities=["termux.command.safe"])
        decision, reason = apply_gate(intent, self.caps)
        self.assertEqual(decision, GateDecision.BLOCKED, reason)

    def test_arbitrary_capability_blocked(self):
        intent = _valid_intent(requested_capabilities=["shell.exec"])
        decision, reason = apply_gate(intent, self.caps)
        self.assertEqual(decision, GateDecision.BLOCKED, reason)

    def test_free_text_capability_blocked(self):
        intent = _valid_intent(requested_capabilities=["rm -rf /"])
        decision, reason = apply_gate(intent, self.caps)
        self.assertEqual(decision, GateDecision.BLOCKED, reason)


class TestGateHumanReviewCases(unittest.TestCase):
    def setUp(self):
        with open(CAPS_PATH) as f:
            self.caps = json.load(f)

    def test_high_risk_human_review(self):
        intent = _valid_intent(risk="high")
        decision, _ = apply_gate(intent, self.caps)
        self.assertEqual(decision, GateDecision.HUMAN_REVIEW)

    def test_execution_gate_human_review(self):
        intent = _valid_intent(execution_gate="human_review")
        decision, _ = apply_gate(intent, self.caps)
        self.assertEqual(decision, GateDecision.HUMAN_REVIEW)

    def test_sandbox_only_gate_not_allowed(self):
        intent = _valid_intent(execution_gate="sandbox_only")
        decision, _ = apply_gate(intent, self.caps)
        self.assertNotEqual(decision, GateDecision.ALLOW)


if __name__ == "__main__":
    unittest.main()
