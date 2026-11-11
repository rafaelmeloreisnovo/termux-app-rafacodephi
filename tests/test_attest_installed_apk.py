"""Host-only adversarial fixtures; NOT physical Android evidence."""
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

SCRIPT = Path(__file__).resolve().parents[1] / "scripts/attest_installed_apk.py"

MOCK_ADB = '''#!/usr/bin/env python3
import os, sys
args = sys.argv[1:]
if args == ["devices"]:
    if os.environ.get("MOCK_DEV_MODE") == "two":
        print("List of devices attached\\nDEVICE_A\\tdevice\\nDEVICE_B\\tdevice")
    else:
        print("List of devices attached\\nDEVICE_SECRET\\tdevice")
    sys.exit(0)
if "getprop" in args:
    print(os.environ.get("MOCK_ABI", "armeabi-v7a"))
elif "pm" in args:
    print("package:/data/app/PRIVATE/base.apk")
    if os.environ.get("MOCK_PM_MODE") == "split":
        print("package:/data/app/PRIVATE/split_config.apk")
elif "sha256sum" in args:
    if os.environ.get("MOCK_HASH_MODE") == "fail": sys.exit(1)
    print(os.environ.get("MOCK_HASH", "0" * 64) + "  /data/app/PRIVATE/base.apk")
else:
    sys.exit(6)
'''


class AttestationTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.apk = self.root / "candidate.apk"
        self.apk.write_bytes(b"an-example-fixture-apk")
        self.sha = hashlib.sha256(self.apk.read_bytes()).hexdigest()
        self.manifest = self.root / "manifest.json"
        self.doc = {
            "schema": "rafcodephi.device-apk-attestation-manifest/v1",
            "ci_run_id": "fixture-only",
            "package": "com.termux.rafacodephi",
            "variants": {"armeabi-v7a": {"sha256": self.sha, "bytes": self.apk.stat().st_size,
                                            "allowed_device_abis": ["armeabi-v7a"]}},
        }
        self.write_manifest()
        self.adb = self.root / "fake-adb"
        self.adb.write_text(MOCK_ADB)
        self.adb.chmod(0o700)

    def write_manifest(self):
        self.manifest.write_text(json.dumps(self.doc))

    def invoke(self, *, overrides=None, candidate=True, receipt_name="receipt.json"):
        env = dict(os.environ, MOCK_HASH=self.sha)
        env.update(overrides or {})
        path = self.root / receipt_name
        cmd = [sys.executable, str(SCRIPT), "--manifest", str(self.manifest),
               "--variant", "armeabi-v7a", "--receipt", str(path), "--adb", str(self.adb)]
        if candidate:
            cmd += ["--candidate-apk", str(self.apk)]
        result = subprocess.run(cmd, env=env, capture_output=True, text=True)
        doc = json.loads(path.read_text()) if path.exists() else None
        return result, doc

    def test_match_scoped_and_redacted(self):
        result, doc = self.invoke()
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(doc["state"], "PASS_SCOPED")
        self.assertFalse(doc["claim_allowed_release"])
        self.assertFalse(doc["claim_allowed_benchmark_binding"])
        self.assertNotIn("DEVICE_SECRET", json.dumps(doc))
        self.assertNotIn("/data/app/PRIVATE", json.dumps(doc))

    def test_negative_device_hash(self):
        result, doc = self.invoke(overrides={"MOCK_HASH": "f" * 64})
        self.assertEqual(result.returncode, 2)
        self.assertEqual(doc["reason"], "INSTALLED_APK_HASH_MISMATCH")

    def test_negative_multiple_devices(self):
        result, doc = self.invoke(overrides={"MOCK_DEV_MODE": "two"})
        self.assertEqual(result.returncode, 2)
        self.assertEqual(doc["reason"], "EXACTLY_ONE_AUTHORIZED_DEVICE_REQUIRED")

    def test_negative_split(self):
        result, doc = self.invoke(overrides={"MOCK_PM_MODE": "split"})
        self.assertEqual(result.returncode, 2)
        self.assertEqual(doc["reason"], "SPLIT_OR_MISSING_APK_PATH")

    def test_negative_abi(self):
        result, doc = self.invoke(overrides={"MOCK_ABI": "arm64-v8a"})
        self.assertEqual(result.returncode, 2)
        self.assertEqual(doc["reason"], "ABI_MISMATCH")

    def test_negative_candidate_bytes(self):
        self.apk.write_bytes(b"tampered")
        result, doc = self.invoke(candidate=True)
        self.assertEqual(result.returncode, 2)
        self.assertEqual(doc["reason"], "CANDIDATE_APK_MISMATCH")

    def test_negative_adb_command(self):
        result, doc = self.invoke(overrides={"MOCK_HASH_MODE": "fail"})
        self.assertEqual(result.returncode, 2)
        self.assertEqual(doc["reason"], "ADB_COMMAND_FAILED")

    def test_no_overwrite_receipt(self):
        first, _ = self.invoke()
        self.assertEqual(first.returncode, 0)
        second, _ = self.invoke()
        self.assertEqual(second.returncode, 3)
        self.assertIn("receipt write failed", second.stderr)

    def test_bad_manifest_never_passes(self):
        self.doc["package"] = "com.termux"
        self.write_manifest()
        result, doc = self.invoke()
        self.assertEqual(result.returncode, 2)
        self.assertEqual(doc["reason"], "PACKAGE_IDENTITY_DRIFT")


if __name__ == "__main__":
    unittest.main()
