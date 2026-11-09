#!/usr/bin/env python3
"""Bounded adversarial tests of the hosted APK source-manifest handoff."""
import os
import subprocess
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
HELPER = ROOT / "scripts/append_apk_source_contract.sh"
PARENT = ROOT / "scripts/build_apk_matrix.sh"
SHA = "d65cd6c2afa9d16196ba0375aaff36833d530b41"


class ApkSourceManifestTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name)
        self.source_dir = self.root / "source-contract"
        self.source_dir.mkdir()
        self.output = self.root / "ARTIFACT_MANIFEST.txt"
        self.output.write_text("artifact_dir=dist/apk-matrix\nrelease_track=internal\n")
        self.valid = (
            "TERMUX_PACKAGES_RAF_REPO=https://github.com/rafaelmeloreisnovo/termux-packages.git\n"
            f"TERMUX_PACKAGES_RAF_RESOLVED_COMMIT={SHA}\n"
            "TERMUX_PACKAGES_RAF_ABIS=armeabi-v7a,arm64-v8a\n"
            "TERMUX_PACKAGES_RAF_ROLE=source-built-packages-bootstrap-and-apt\n"
        )
        self.source = self.source_dir / "TERMUX_PACKAGES_RAFCODEPHI_SOURCE.env"
        self.source.write_text(self.valid)
        self.env = dict(os.environ, TERMUX_PACKAGES_RAF_MANIFEST_DIR=str(self.source_dir))

    def run_helper(self, extra=None):
        return subprocess.run(
            ["bash", str(HELPER), *(extra if extra is not None else [str(self.output)])],
            cwd=self.root, env=self.env, text=True, capture_output=True, check=False
        )

    def test_positive_exact_producer_attached_not_decoy(self):
        decoy = self.root / "dist/source-contract"
        decoy.mkdir(parents=True)
        (decoy / "TERMUX_PACKAGES_RAFCODEPHI_SOURCE.env").write_text("DECOY=1\n")
        result = self.run_helper()
        self.assertEqual(result.returncode, 0, result.stderr)
        content = self.output.read_text()
        self.assertIn(f"TERMUX_PACKAGES_RAF_RESOLVED_COMMIT={SHA}", content)
        self.assertNotIn("DECOY=1", content)
        self.assertEqual(content.count("TERMUX_PACKAGES_RAF_REPO="), 1)

    def test_missing_source_blocks_without_partial_write(self):
        self.source.unlink()
        before = self.output.read_bytes()
        result = self.run_helper()
        self.assertNotEqual(result.returncode, 0)
        self.assertEqual(self.output.read_bytes(), before)

    def test_malformed_sha_blocks_without_partial_write(self):
        self.source.write_text(self.valid.replace(SHA, "TOKEN_VAZIO"))
        before = self.output.read_bytes()
        self.assertNotEqual(self.run_helper().returncode, 0)
        self.assertEqual(self.output.read_bytes(), before)

    def test_missing_arm32_blocks_without_partial_write(self):
        self.source.write_text(self.valid.replace("armeabi-v7a,arm64-v8a", "arm64-v8a"))
        before = self.output.read_bytes()
        self.assertNotEqual(self.run_helper().returncode, 0)
        self.assertEqual(self.output.read_bytes(), before)

    def test_replay_is_rejected_no_duplicate(self):
        self.assertEqual(self.run_helper().returncode, 0)
        before = self.output.read_bytes()
        self.assertNotEqual(self.run_helper().returncode, 0)
        self.assertEqual(self.output.read_bytes(), before)

    def test_parent_calls_helper_and_no_silent_old_relative_cat(self):
        parent = PARENT.read_text()
        self.assertIn('bash scripts/append_apk_source_contract.sh "${OUT_DIR}/ARTIFACT_MANIFEST.txt"', parent)
        self.assertNotIn("cat dist/source-contract/TERMUX_PACKAGES_RAFCODEPHI_SOURCE.env", parent)
        self.assertNotIn('|| true;\n  echo "signed_release_apks_required=', parent)


if __name__ == "__main__":
    unittest.main(verbosity=2)
