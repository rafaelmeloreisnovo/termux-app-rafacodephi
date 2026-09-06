#!/usr/bin/env python3
from __future__ import annotations

import importlib.util
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
MODULE_PATH = ROOT / "scripts" / "inject_freestanding_gate_into_bootstrap.py"
spec = importlib.util.spec_from_file_location("freestanding_inject", MODULE_PATH)
module = importlib.util.module_from_spec(spec)
assert spec and spec.loader
spec.loader.exec_module(module)


PARENT = """schema=rafcodephi.real-bootstrap-sourcebuild/v1
package_name=com.termux.rafacodephi
prefix=/data/data/com.termux.rafacodephi/files/usr
bridge_allowed=false
legacy_prefix_allowed=false
claim_allowed_device_runtime=false
device_runtime_proof=TOKEN_VAZIO
sha256_arm={arm}
sha256_aarch64={aarch64}
"""


class ChildManifestTest(unittest.TestCase):
    def reports(self) -> dict[str, dict]:
        return {
            "arm": {"output_zip_sha256": "1" * 64, "gate_sha256": "a" * 64},
            "aarch64": {"output_zip_sha256": "2" * 64, "gate_sha256": "b" * 64},
        }

    def test_child_manifest_updates_pair_hashes_and_preserves_parent_custody(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            parent = root / "parent.txt"
            parent.write_text(PARENT.format(arm="3" * 64, aarch64="4" * 64), encoding="utf-8")
            lines, fields = module.parse_manifest(parent)
            self.assertEqual("com.termux.rafacodephi", fields["package_name"])
            parent_sha = module.digest(parent)
            child = root / "child.txt"
            module.write_child_manifest(lines, parent_sha, self.reports(), child)
            text = child.read_text(encoding="utf-8")
            self.assertIn("sha256_arm=" + "1" * 64, text)
            self.assertIn("sha256_aarch64=" + "2" * 64, text)
            self.assertIn("freestanding_parent_manifest_sha256=" + parent_sha, text)
            self.assertIn("freestanding_gate_sha256_arm=" + "a" * 64, text)
            self.assertIn("freestanding_gate_sha256_aarch64=" + "b" * 64, text)
            self.assertIn("freestanding_device_state=TOKEN_VAZIO", text)
            self.assertIn("freestanding_claim_allowed=false", text)
            self.assertNotIn("sha256_arm=" + "3" * 64, text)
            self.assertNotIn("sha256_aarch64=" + "4" * 64, text)

    def test_parse_manifest_rejects_widened_claim_boundary(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "bad.txt"
            path.write_text(
                PARENT.format(arm="3" * 64, aarch64="4" * 64).replace(
                    "claim_allowed_device_runtime=false", "claim_allowed_device_runtime=true"
                ),
                encoding="utf-8",
            )
            with self.assertRaises(SystemExit):
                module.parse_manifest(path)

    def test_child_manifest_requires_both_declared_arch_hashes(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            lines = [
                "schema=rafcodephi.real-bootstrap-sourcebuild/v1",
                "sha256_arm=" + "3" * 64,
            ]
            with self.assertRaises(SystemExit):
                module.write_child_manifest(lines, "f" * 64, self.reports(), root / "child.txt")


if __name__ == "__main__":
    unittest.main()
