import json
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[2]
SCRIPT = REPO_ROOT / "scripts" / "vertical_slice" / "run_readonly_flow.sh"
HAM_SCRIPT = REPO_ROOT / "scripts" / "vertical_slice" / "run_ham_readonly_flow.sh"


def valid_intent() -> dict:
    return {
        "schema": "rafaelia.intent.v1",
        "intent_id": "receipt-test-0001",
        "action": "git.status",
        "target": {"repo_path": "local"},
        "inputs": [],
        "constraints": [{"key": "read_only", "value": True}],
        "evidence_refs": [{"chunk_id": "chunk-test-01"}],
        "requested_capabilities": ["git.read", "git.diff"],
        "risk": "low",
        "execution_gate": "allow",
    }


@unittest.skipUnless(shutil.which("git"), "git is required")
class TestReadonlyFlowReceipt(unittest.TestCase):
    def run_flow(self, work_dir: Path, result_root: Path) -> subprocess.CompletedProcess[str]:
        intent_path = result_root.parent / "intent.json"
        intent_path.write_text(json.dumps(valid_intent()), encoding="utf-8")
        return subprocess.run(
            ["bash", str(SCRIPT), str(intent_path), str(work_dir), str(result_root)],
            check=False,
            text=True,
            capture_output=True,
        )

    @staticmethod
    def results(result_root: Path) -> list[Path]:
        return sorted(result_root.glob("run-*/execution_result.json"))

    def init_git_repository(self, work_dir: Path) -> str:
        subprocess.run(["git", "init", str(work_dir)], check=True, capture_output=True, text=True)
        subprocess.run(["git", "-C", str(work_dir), "config", "user.email", "tests@example.invalid"], check=True)
        subprocess.run(["git", "-C", str(work_dir), "config", "user.name", "Receipt Test"], check=True)
        (work_dir / "README.md").write_text("receipt test\n", encoding="utf-8")
        subprocess.run(["git", "-C", str(work_dir), "add", "README.md"], check=True)
        subprocess.run(["git", "-C", str(work_dir), "commit", "-m", "fixture"], check=True, capture_output=True, text=True)
        return subprocess.run(
            ["git", "-C", str(work_dir), "rev-parse", "HEAD"],
            check=True,
            text=True,
            capture_output=True,
        ).stdout.strip()

    def test_non_git_directory_is_a_failure_receipt(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            work_dir = root / "not-a-repository"
            result_root = root / "receipts"
            work_dir.mkdir()

            completed = self.run_flow(work_dir, result_root)

            self.assertNotEqual(completed.returncode, 0, completed.stdout + completed.stderr)
            [receipt_path] = self.results(result_root)
            receipt = json.loads(receipt_path.read_text(encoding="utf-8"))
            self.assertEqual(receipt["final_state"], "failure")
            self.assertEqual(receipt["exit_code"], completed.returncode)
            self.assertEqual(receipt["evidence_state"], "partial")
            self.assertTrue(receipt["target_commit"].startswith("TOKEN_VAZIO_"))
            self.assertTrue(any(item["exit_code"] != 0 for item in receipt["command_results"]))
            self.assertNotIn("/tmp/vs_err_", receipt_path.read_text(encoding="utf-8"))

    def test_git_repository_records_true_commit_and_success(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            work_dir = root / "repository"
            result_root = root / "receipts"
            work_dir.mkdir()
            commit = self.init_git_repository(work_dir)

            completed = self.run_flow(work_dir, result_root)

            self.assertEqual(completed.returncode, 0, completed.stdout + completed.stderr)
            [receipt_path] = self.results(result_root)
            receipt = json.loads(receipt_path.read_text(encoding="utf-8"))
            self.assertEqual(receipt["final_state"], "success")
            self.assertEqual(receipt["exit_code"], 0)
            self.assertEqual(receipt["target_commit"], commit)
            self.assertEqual(receipt["evidence_state"], "complete")
            self.assertFalse(receipt["claim_allowed"])
            self.assertEqual(len(receipt["command_results"]), 4)
            try:
                import jsonschema
            except ImportError:
                jsonschema = None
            if jsonschema is not None:
                schema = json.loads(
                    (REPO_ROOT / "docs" / "contracts" / "execution_result.schema.json").read_text(
                        encoding="utf-8"
                    )
                )
                jsonschema.validate(receipt, schema)

    def test_each_run_has_a_distinct_immutable_directory(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            work_dir = root / "repository"
            result_root = root / "receipts"
            work_dir.mkdir()
            self.init_git_repository(work_dir)

            first = self.run_flow(work_dir, result_root)
            second = self.run_flow(work_dir, result_root)

            self.assertEqual(first.returncode, 0, first.stdout + first.stderr)
            self.assertEqual(second.returncode, 0, second.stdout + second.stderr)
            receipts = self.results(result_root)
            self.assertEqual(len(receipts), 2)
            self.assertNotEqual(receipts[0].parent.name, receipts[1].parent.name)
            self.assertNotEqual(
                json.loads(receipts[0].read_text(encoding="utf-8"))["run_id"],
                json.loads(receipts[1].read_text(encoding="utf-8"))["run_id"],
            )

    def test_ham_preserves_a_base_flow_failure_receipt(self) -> None:
        ham_request = {
            "schema": "raf.human-ai.middleware.v1",
            "request_id": "ham-test-0001",
            "execution": {
                "target_repository": "rafaelmeloreisnovo/termux-app-rafacodephi",
                "effect_class": "READ_ONLY",
            },
            "ai_lane": {
                "may_execute": False,
                "may_finalize": False,
                "may_expand_scope": False,
            },
            "human_lane": {
                "decision": "APPROVE_BOUNDED",
                "consent_state": "APPROVED",
            },
            "people": {"human_final_decision": True},
            "risk": {"level": "LOW"},
            "friction": {
                "stop_on_no_new_evidence": True,
                "loop_budget": 1,
                "current_loop": 1,
            },
            "data_boundary": {
                "destination_visibility": "PRIVATE",
                "raw_data_export": False,
            },
        }
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            work_dir = root / "not-a-repository"
            result_root = root / "receipts"
            intent_path = root / "intent.json"
            ham_path = root / "ham.json"
            work_dir.mkdir()
            intent_path.write_text(json.dumps(valid_intent()), encoding="utf-8")
            ham_path.write_text(json.dumps(ham_request), encoding="utf-8")

            completed = subprocess.run(
                [
                    "bash",
                    str(HAM_SCRIPT),
                    str(ham_path),
                    str(intent_path),
                    str(work_dir),
                    str(result_root),
                ],
                check=False,
                text=True,
                capture_output=True,
            )

            self.assertNotEqual(completed.returncode, 0, completed.stdout + completed.stderr)
            [receipt_path] = sorted(result_root.glob("run-*/ham_execution_receipt.json"))
            receipt = json.loads(receipt_path.read_text(encoding="utf-8"))
            self.assertEqual(receipt["final_state"], "failure")
            self.assertEqual(receipt["base_exit_code"], completed.returncode)
            self.assertEqual(receipt["execution_result"]["final_state"], "failure")

    def test_policy_declares_every_readonly_probe_command(self) -> None:
        policy = json.loads(
            (REPO_ROOT / "internal" / "governance" / "policy.json").read_text(encoding="utf-8")
        )
        v1_rule = next(rule for rule in policy["rules"] if rule["id"] == "RULE-003")
        self.assertEqual(
            v1_rule["allowed_v1_commands"],
            [
                ["git", "rev-parse", "--is-inside-work-tree"],
                ["git", "rev-parse", "HEAD"],
                ["git", "status", "--short", "--branch"],
                ["git", "diff", "--no-ext-diff", "--stat"],
            ],
        )


if __name__ == "__main__":
    unittest.main()
