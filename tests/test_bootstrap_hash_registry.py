"""Regression tests for the authorial bootstrap ZIP registry."""
import importlib.util
import json
import tempfile
import unittest
import zipfile
from pathlib import Path

PATH = Path(__file__).resolve().parents[1] / "tools/bootstrap_hash_registry.py"
spec = importlib.util.spec_from_file_location("registry_under_test", PATH)
registry = importlib.util.module_from_spec(spec)
spec.loader.exec_module(registry)

class BootstrapRegistryTest(unittest.TestCase):
    def test_verified_archive_has_exact_hash_and_abi(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "candidate.zip"
            with zipfile.ZipFile(path, "w") as archive:
                archive.writestr("SYMLINKS.txt", "")
                archive.writestr("BOOTSTRAP_PROFILE.json", json.dumps({
                    "arch": "arm", "profile": "bridge", "claim_allowed": False
                }))
                archive.writestr("bin/sh", "shell")
                archive.writestr("bin/pkg", "packages")
            observed = registry.inspect(path, "arm")
            self.assertEqual(observed["structure"], "PASS")
            self.assertEqual(observed["zip_crc"], "PASS")
            self.assertEqual(len(observed["sha256"]), 64)
            with self.assertRaises(ValueError):
                registry.inspect(path, "aarch64")

    def test_registry_requires_explicit_review(self):
        registry.verify_registry({"schema": registry.SCHEMA, "entries": []})
        unreviewed = {"schema": registry.SCHEMA, "entries": [{
            "abi": "arm", "sha256": "0" * 64, "status": "PENDING_OWNER_REVIEW"
        }]}
        with self.assertRaises(ValueError):
            registry.verify_registry(unreviewed)

if __name__ == "__main__":
    unittest.main()
