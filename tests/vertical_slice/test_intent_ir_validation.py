"""
tests/vertical_slice/test_intent_ir_validation.py

Validates the intent_ir.schema.json contract: required fields, enum values,
valid and invalid payloads.  No network calls, no subprocess spawning.
"""
import json
import os
import unittest
from pathlib import Path

REPO_ROOT = Path(__file__).parent.parent.parent
SCHEMA_PATH = REPO_ROOT / "docs" / "contracts" / "intent_ir.schema.json"

# ─────────────────────────────────────────────────────────────
# Helpers
# ─────────────────────────────────────────────────────────────

REQUIRED_FIELDS = [
    "schema",
    "intent_id",
    "action",
    "target",
    "inputs",
    "constraints",
    "evidence_refs",
    "requested_capabilities",
    "risk",
    "execution_gate",
]

VALID_RISKS = {"low", "medium", "high", "critical"}
VALID_GATES = {"allow", "sandbox_only", "human_review", "blocked"}
VALID_CAPS  = {"git.read", "git.diff", "filesystem.read", "termux.command.safe"}


def _valid_intent(**overrides):
    base = {
        "schema": "rafaelia.intent.v1",
        "intent_id": "test-intent-0001",
        "action": "git.status",
        "target": {"repo_path": "/tmp/testrepo"},
        "inputs": [],
        "constraints": [{"key": "read_only", "value": True}],
        "evidence_refs": [],
        "requested_capabilities": ["git.read"],
        "risk": "low",
        "execution_gate": "allow",
        "created_at": "2026-07-10T21:00:00Z",
        "source_bundle_id": None,
    }
    base.update(overrides)
    return base


def _validate(intent: dict) -> list[str]:
    """Returns list of validation errors (empty = valid)."""
    errors = []
    for field in REQUIRED_FIELDS:
        if field not in intent:
            errors.append(f"missing required field: {field}")
    if intent.get("schema") != "rafaelia.intent.v1":
        errors.append(f"schema must be 'rafaelia.intent.v1', got: {intent.get('schema')!r}")
    if intent.get("risk") not in VALID_RISKS:
        errors.append(f"risk must be one of {VALID_RISKS}, got: {intent.get('risk')!r}")
    if intent.get("execution_gate") not in VALID_GATES:
        errors.append(f"execution_gate must be one of {VALID_GATES}, got: {intent.get('execution_gate')!r}")
    for cap in intent.get("requested_capabilities", []):
        if cap not in VALID_CAPS:
            errors.append(f"unknown capability: {cap!r}")
    return errors


# ─────────────────────────────────────────────────────────────
# Tests
# ─────────────────────────────────────────────────────────────

class TestIntentIRSchemaFile(unittest.TestCase):
    def test_schema_file_exists(self):
        self.assertTrue(SCHEMA_PATH.exists(), f"Schema not found: {SCHEMA_PATH}")

    def test_schema_file_is_valid_json(self):
        with open(SCHEMA_PATH) as f:
            schema = json.load(f)
        self.assertIsInstance(schema, dict)

    def test_schema_has_required_properties(self):
        with open(SCHEMA_PATH) as f:
            schema = json.load(f)
        props = schema.get("properties", {})
        for field in REQUIRED_FIELDS:
            self.assertIn(field, props, f"Schema missing property: {field}")

    def test_schema_required_list_complete(self):
        with open(SCHEMA_PATH) as f:
            schema = json.load(f)
        required_in_schema = set(schema.get("required", []))
        for field in REQUIRED_FIELDS:
            self.assertIn(field, required_in_schema,
                          f"'{field}' not in schema 'required' list")


class TestIntentIRValidPayloads(unittest.TestCase):
    def test_minimal_valid_intent(self):
        intent = _valid_intent()
        errors = _validate(intent)
        self.assertEqual(errors, [], f"Unexpected errors: {errors}")

    def test_all_risk_levels_accepted(self):
        for risk in VALID_RISKS:
            with self.subTest(risk=risk):
                intent = _valid_intent(risk=risk)
                errors = _validate(intent)
                self.assertNotIn(
                    f"risk must be one of {VALID_RISKS}, got: {risk!r}", errors
                )

    def test_all_gate_values_accepted(self):
        for gate in VALID_GATES:
            with self.subTest(gate=gate):
                intent = _valid_intent(execution_gate=gate)
                errors = _validate(intent)
                self.assertNotIn(
                    f"execution_gate must be one of {VALID_GATES}, got: {gate!r}", errors
                )

    def test_all_known_capabilities_accepted(self):
        intent = _valid_intent(requested_capabilities=list(VALID_CAPS))
        errors = _validate(intent)
        unknown_errors = [e for e in errors if "unknown capability" in e]
        self.assertEqual(unknown_errors, [])

    def test_empty_capabilities_list_accepted(self):
        intent = _valid_intent(requested_capabilities=[])
        errors = _validate(intent)
        self.assertEqual(errors, [])


class TestIntentIRInvalidPayloads(unittest.TestCase):
    def test_missing_required_field_schema(self):
        intent = _valid_intent()
        del intent["schema"]
        errors = _validate(intent)
        self.assertTrue(any("schema" in e for e in errors), errors)

    def test_missing_required_field_intent_id(self):
        intent = _valid_intent()
        del intent["intent_id"]
        errors = _validate(intent)
        self.assertTrue(any("intent_id" in e for e in errors), errors)

    def test_missing_required_field_risk(self):
        intent = _valid_intent()
        del intent["risk"]
        errors = _validate(intent)
        self.assertTrue(any("risk" in e for e in errors), errors)

    def test_wrong_schema_value(self):
        intent = _valid_intent(schema="wrong.schema.v99")
        errors = _validate(intent)
        self.assertTrue(any("schema" in e for e in errors), errors)

    def test_invalid_risk_value(self):
        intent = _valid_intent(risk="extreme")
        errors = _validate(intent)
        self.assertTrue(any("risk" in e for e in errors), errors)

    def test_invalid_gate_value(self):
        intent = _valid_intent(execution_gate="maybe")
        errors = _validate(intent)
        self.assertTrue(any("execution_gate" in e for e in errors), errors)

    def test_unknown_capability_rejected(self):
        intent = _valid_intent(requested_capabilities=["git.write"])
        errors = _validate(intent)
        self.assertTrue(any("git.write" in e for e in errors), errors)

    def test_multiple_missing_fields(self):
        intent = {}
        errors = _validate(intent)
        self.assertGreaterEqual(len(errors), len(REQUIRED_FIELDS))

    def test_free_text_as_action_still_validates_other_fields(self):
        """Action is free-text but other fields must still satisfy schema."""
        intent = _valid_intent(action="rm -rf /")
        # action field itself has no enum restriction at schema level (governance blocks it)
        errors = _validate(intent)
        # Should have no schema errors — governance gate handles blocking
        self.assertEqual(errors, [])


if __name__ == "__main__":
    unittest.main()
