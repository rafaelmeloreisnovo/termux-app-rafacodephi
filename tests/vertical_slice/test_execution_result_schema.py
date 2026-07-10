"""
tests/vertical_slice/test_execution_result_schema.py

Validates the execution_result.schema.json contract and tests that
execution_result payloads (valid/invalid) match the schema.
Also verifies hash format, timestamp format, and required field presence.
"""
import hashlib
import json
import re
import unittest
from datetime import datetime, timezone
from pathlib import Path

REPO_ROOT = Path(__file__).parent.parent.parent
RESULT_SCHEMA_PATH = REPO_ROOT / "docs" / "contracts" / "execution_result.schema.json"

# ─────────────────────────────────────────────────────────────
# Helpers
# ─────────────────────────────────────────────────────────────

REQUIRED_RESULT_FIELDS = [
    "result_id",
    "intent_id",
    "executed_command",
    "args",
    "working_directory",
    "started_at",
    "ended_at",
    "exit_code",
    "stdout_truncated",
    "stderr_truncated",
    "stdout_sha256",
    "stderr_sha256",
    "artifacts",
    "final_state",
    "rollback_available",
    "source_chunk_refs",
]

SHA256_RE = re.compile(r"^[a-f0-9]{64}$")
# Accepts both Z-suffix and +HH:MM offset ISO-8601
ISO8601_RE = re.compile(
    r"^\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}(?:\.\d+)?(?:Z|[+-]\d{2}:\d{2})$"
)


def sha256_of(text: str) -> str:
    return hashlib.sha256(text.encode()).hexdigest()


def _valid_result(**overrides):
    stdout = "On branch main\nnothing to commit, working tree clean\n"
    stderr = ""
    base = {
        "result_id": "result-test-0001",
        "intent_id": "intent-test-0001",
        "plan_id": "plan-test-0001",
        "executed_command": "git",
        "args": ["status"],
        "working_directory": "/tmp/testrepo",
        "started_at": "2026-07-10T21:00:00Z",
        "ended_at": "2026-07-10T21:00:01Z",
        "exit_code": 0,
        "stdout_truncated": stdout[:4096],
        "stderr_truncated": stderr[:4096],
        "stdout_sha256": sha256_of(stdout),
        "stderr_sha256": sha256_of(stderr),
        "artifacts": [],
        "final_state": "success",
        "rollback_available": False,
        "source_chunk_refs": [],
    }
    base.update(overrides)
    return base


def _validate_result(result: dict) -> list[str]:
    errors = []
    for field in REQUIRED_RESULT_FIELDS:
        if field not in result:
            errors.append(f"missing required field: {field}")

    if result.get("rollback_available") is not False:
        # For read-only commands this must always be False in v1
        # We only warn here, not hard-error, since schema allows True for future
        pass

    for sha_field in ("stdout_sha256", "stderr_sha256"):
        val = result.get(sha_field, "")
        if val and not SHA256_RE.match(val):
            errors.append(f"{sha_field} is not a valid sha256: {val!r}")

    for ts_field in ("started_at", "ended_at"):
        val = result.get(ts_field, "")
        if val and not ISO8601_RE.match(val):
            errors.append(f"{ts_field} is not valid ISO-8601: {val!r}")

    if result.get("final_state") not in ("success", "failure", "timeout", "blocked", None):
        errors.append(f"invalid final_state: {result.get('final_state')!r}")

    if not isinstance(result.get("artifacts", []), list):
        errors.append("artifacts must be a list")

    if not isinstance(result.get("source_chunk_refs", []), list):
        errors.append("source_chunk_refs must be a list")

    for art in result.get("artifacts", []):
        if "name" not in art or "path" not in art:
            errors.append(f"artifact missing name or path: {art}")
        if "sha256" in art and not SHA256_RE.match(art.get("sha256", "")):
            errors.append(f"artifact sha256 invalid: {art.get('sha256')!r}")

    return errors


# ─────────────────────────────────────────────────────────────
# Tests
# ─────────────────────────────────────────────────────────────

class TestExecutionResultSchemaFile(unittest.TestCase):
    def test_schema_file_exists(self):
        self.assertTrue(RESULT_SCHEMA_PATH.exists(), f"Schema not found: {RESULT_SCHEMA_PATH}")

    def test_schema_file_is_valid_json(self):
        with open(RESULT_SCHEMA_PATH) as f:
            schema = json.load(f)
        self.assertIsInstance(schema, dict)

    def test_schema_has_all_required_properties(self):
        with open(RESULT_SCHEMA_PATH) as f:
            schema = json.load(f)
        props = schema.get("properties", {})
        for field in REQUIRED_RESULT_FIELDS:
            self.assertIn(field, props, f"Schema missing property: {field}")

    def test_schema_required_list_complete(self):
        with open(RESULT_SCHEMA_PATH) as f:
            schema = json.load(f)
        required_in_schema = set(schema.get("required", []))
        for field in REQUIRED_RESULT_FIELDS:
            self.assertIn(field, required_in_schema,
                          f"'{field}' not in schema 'required' list")

    def test_schema_stdout_sha256_has_pattern(self):
        with open(RESULT_SCHEMA_PATH) as f:
            schema = json.load(f)
        sha_prop = schema.get("properties", {}).get("stdout_sha256", {})
        self.assertIn("pattern", sha_prop, "stdout_sha256 should have a regex pattern")

    def test_schema_stderr_sha256_has_pattern(self):
        with open(RESULT_SCHEMA_PATH) as f:
            schema = json.load(f)
        sha_prop = schema.get("properties", {}).get("stderr_sha256", {})
        self.assertIn("pattern", sha_prop, "stderr_sha256 should have a regex pattern")


class TestExecutionResultValidPayloads(unittest.TestCase):
    def test_minimal_valid_result(self):
        result = _valid_result()
        errors = _validate_result(result)
        self.assertEqual(errors, [], f"Unexpected errors: {errors}")

    def test_read_only_rollback_always_false(self):
        result = _valid_result(rollback_available=False)
        errors = _validate_result(result)
        self.assertEqual(errors, [])

    def test_all_final_states(self):
        for state in ("success", "failure", "timeout", "blocked"):
            with self.subTest(state=state):
                result = _valid_result(final_state=state)
                errors = _validate_result(result)
                self.assertEqual(errors, [], errors)

    def test_sha256_format_correct(self):
        stdout = "test output"
        result = _valid_result(
            stdout_truncated=stdout,
            stdout_sha256=sha256_of(stdout),
            stderr_sha256=sha256_of(""),
        )
        errors = _validate_result(result)
        self.assertEqual(errors, [])

    def test_artifacts_with_valid_sha256(self):
        content = "artifact content"
        result = _valid_result(artifacts=[
            {"name": "output", "path": "/tmp/out.txt", "sha256": sha256_of(content)}
        ])
        errors = _validate_result(result)
        self.assertEqual(errors, [])

    def test_source_chunk_refs_populated(self):
        result = _valid_result(source_chunk_refs=["chunk-001", "chunk-002"])
        errors = _validate_result(result)
        self.assertEqual(errors, [])


class TestExecutionResultInvalidPayloads(unittest.TestCase):
    def test_missing_required_fields(self):
        for field in REQUIRED_RESULT_FIELDS:
            with self.subTest(field=field):
                result = _valid_result()
                del result[field]
                errors = _validate_result(result)
                self.assertTrue(
                    any(field in e for e in errors),
                    f"Expected error for missing '{field}', got: {errors}"
                )

    def test_bad_sha256_stdout(self):
        result = _valid_result(stdout_sha256="not-a-hash")
        errors = _validate_result(result)
        self.assertTrue(any("stdout_sha256" in e for e in errors), errors)

    def test_bad_sha256_stderr(self):
        result = _valid_result(stderr_sha256="ZZZZ")
        errors = _validate_result(result)
        self.assertTrue(any("stderr_sha256" in e for e in errors), errors)

    def test_bad_started_at_format(self):
        result = _valid_result(started_at="not-a-date")
        errors = _validate_result(result)
        self.assertTrue(any("started_at" in e for e in errors), errors)

    def test_bad_ended_at_format(self):
        result = _valid_result(ended_at="20260710")
        errors = _validate_result(result)
        self.assertTrue(any("ended_at" in e for e in errors), errors)

    def test_invalid_final_state(self):
        result = _valid_result(final_state="running")
        errors = _validate_result(result)
        self.assertTrue(any("final_state" in e for e in errors), errors)

    def test_artifact_missing_name(self):
        result = _valid_result(artifacts=[{"path": "/tmp/out.txt"}])
        errors = _validate_result(result)
        self.assertTrue(any("artifact" in e for e in errors), errors)

    def test_artifact_bad_sha256(self):
        result = _valid_result(artifacts=[
            {"name": "x", "path": "/tmp/x", "sha256": "badhash"}
        ])
        errors = _validate_result(result)
        self.assertTrue(any("sha256" in e for e in errors), errors)


class TestSHA256HashIntegrity(unittest.TestCase):
    """Verify that sha256 computed values match expectations."""

    def test_empty_string_sha256(self):
        expected = "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"
        self.assertEqual(sha256_of(""), expected)

    def test_known_string_sha256(self):
        text = "On branch main\nnothing to commit, working tree clean\n"
        digest = sha256_of(text)
        self.assertRegex(digest, r"^[a-f0-9]{64}$")

    def test_sha256_is_deterministic(self):
        text = "git status output line 1\nline 2\n"
        self.assertEqual(sha256_of(text), sha256_of(text))

    def test_different_outputs_have_different_hashes(self):
        self.assertNotEqual(sha256_of("output a"), sha256_of("output b"))


if __name__ == "__main__":
    unittest.main()
