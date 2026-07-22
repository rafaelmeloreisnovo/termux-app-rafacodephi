import copy
import json
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SCRIPT = ROOT / "scripts" / "vertical_slice" / "run_ham_readonly_flow.sh"
HAM_FIXTURE = ROOT / "internal" / "orchestrator" / "examples" / "ham_readonly_request.json"
INTENT_FIXTURE = ROOT / "internal" / "orchestrator" / "examples" / "ham_readonly_intent.json"


@unittest.skipUnless(shutil.which("git") and shutil.which("bash") and shutil.which("python3"), "requires git bash python3")
class HamReadonlyFlowTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.work = Path(self.temp.name)
        subprocess.run(["git", "init", "-q", str(self.work / "repo")], check=True)
        (self.work / "repo" / "README.txt").write_text("bounded fixture\n", encoding="utf-8")

    def tearDown(self):
        self.temp.cleanup()

    def run_flow(self, request):
        request_path = self.work / "request.json"
        request_path.write_text(json.dumps(request, ensure_ascii=False, indent=2), encoding="utf-8")
        return subprocess.run(
            [
                "bash",
                str(SCRIPT),
                str(request_path),
                str(INTENT_FIXTURE),
                str(self.work / "repo"),
            ],
            cwd=self.work,
            text=True,
            capture_output=True,
            timeout=30,
            check=False,
        )

    def fixture(self):
        return json.loads(HAM_FIXTURE.read_text(encoding="utf-8"))

    def test_bounded_read_emits_hashed_receipt(self):
        result = self.run_flow(self.fixture())
        self.assertEqual(0, result.returncode, result.stderr)
        receipt_path = self.work / "ham_execution_receipt.json"
        self.assertTrue(receipt_path.is_file())
        receipt = json.loads(receipt_path.read_text(encoding="utf-8"))
        self.assertEqual("raf.human-ai.execution-receipt.v1", receipt["schema"])
        self.assertEqual("ALLOW_BOUNDED", receipt["gate_decision"])
        self.assertEqual("READ_ONLY", receipt["effect_class"])
        self.assertFalse(receipt["claim_allowed"])
        self.assertTrue(receipt["source_preserved"])
        self.assertRegex(receipt["request_sha256"], r"^[0-9a-f]{64}$")
        self.assertRegex(receipt["receipt_sha256"], r"^[0-9a-f]{64}$")
        self.assertRegex(receipt["execution_result"]["stdout_sha256"], r"^[0-9a-f]{64}$")

    def test_ai_execution_authority_is_blocked_before_base_flow(self):
        request = self.fixture()
        request["ai_lane"]["may_execute"] = True
        result = self.run_flow(request)
        self.assertEqual(5, result.returncode)
        self.assertIn("AI_EXECUTE", result.stderr)
        self.assertFalse((self.work / "execution_result.json").exists())

    def test_write_effect_is_blocked(self):
        request = self.fixture()
        request["execution"]["effect_class"] = "GIT_WRITE"
        result = self.run_flow(request)
        self.assertEqual(5, result.returncode)
        self.assertIn("EFFECT", result.stderr)
        self.assertFalse((self.work / "execution_result.json").exists())

    def test_secret_material_is_blocked(self):
        request = self.fixture()
        request["access_token"] = "must-not-enter-runtime"
        result = self.run_flow(request)
        self.assertEqual(5, result.returncode)
        self.assertIn("SECRET", result.stderr)
        self.assertFalse((self.work / "execution_result.json").exists())

    def test_wrong_repository_is_blocked(self):
        request = self.fixture()
        request["execution"]["target_repository"] = "rafaelmeloreisnovo/RafPolimata"
        result = self.run_flow(request)
        self.assertEqual(5, result.returncode)
        self.assertIn("TARGET", result.stderr)


if __name__ == "__main__":
    unittest.main()
