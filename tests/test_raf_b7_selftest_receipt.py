#!/usr/bin/env python3
from __future__ import annotations

import hashlib
import json
import os
import subprocess
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SCRIPT = ROOT / "scripts" / "run_raf_b7_selftest_receipt.sh"


class RafB7SelftestReceiptTest(unittest.TestCase):
    @staticmethod
    def only_receipt(out_dir: Path) -> Path:
        receipts = sorted(out_dir.glob("RAF-B7-SELFTEST-*.json"))
        if len(receipts) != 1:
            raise AssertionError(f"expected one receipt, found {receipts}")
        return receipts[0]

    @staticmethod
    def run_wrapper(
        out_dir: Path,
        tmp_dir: Path,
        compiler: str,
    ) -> subprocess.CompletedProcess[str]:
        environment = os.environ.copy()
        environment["CC"] = compiler
        environment["TMPDIR"] = str(tmp_dir)
        environment.pop("PREFIX", None)
        return subprocess.run(
            ["sh", str(SCRIPT), str(out_dir)],
            cwd=ROOT,
            env=environment,
            text=True,
            capture_output=True,
            check=False,
        )

    def test_success_receipt_hashes_sources_binary_and_output(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            out_dir = root / "receipts"
            tmp_dir = root / "tmp"
            tmp_dir.mkdir()

            result = self.run_wrapper(out_dir, tmp_dir, os.environ.get("CC", "cc"))
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertIn("PASS bytes=8192", result.stdout)

            receipt_path = self.only_receipt(out_dir)
            receipt = json.loads(receipt_path.read_text(encoding="utf-8"))
            self.assertEqual(receipt["checks"]["all_inputs_present"], "PASS")
            self.assertEqual(receipt["checks"]["compile_and_execute"], "PASS")
            self.assertEqual(receipt["checks"]["output_contract"], "PASS")
            self.assertEqual(receipt["checks"]["binary_hash"], "PASS")
            self.assertEqual(receipt["execution"]["exit_code"], 0)
            self.assertEqual(receipt["execution"]["parsed"]["bytes"], 8192)
            self.assertTrue(receipt["selftest_internal_attestation_exercised"])
            self.assertFalse(receipt["claim_allowed"])

            binary = receipt["execution"]["binary"]
            self.assertTrue(binary["present"])
            self.assertRegex(binary["sha256"], r"^[0-9a-f]{64}$")

            sidecar = Path(f"{receipt_path}.sha256")
            self.assertTrue(sidecar.is_file())
            self.assertEqual(
                sidecar.read_text(encoding="utf-8").split()[0],
                hashlib.sha256(receipt_path.read_bytes()).hexdigest(),
            )

    def test_failed_compiler_still_writes_failure_receipt(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            out_dir = root / "receipts"
            tmp_dir = root / "tmp"
            tmp_dir.mkdir()

            result = self.run_wrapper(out_dir, tmp_dir, "false")
            self.assertNotEqual(result.returncode, 0)

            receipt_path = self.only_receipt(out_dir)
            receipt = json.loads(receipt_path.read_text(encoding="utf-8"))
            self.assertNotEqual(receipt["execution"]["exit_code"], 0)
            self.assertEqual(receipt["checks"]["compile_and_execute"], "FAIL")
            self.assertEqual(receipt["checks"]["output_contract"], "FAIL")
            self.assertFalse(receipt["selftest_internal_attestation_exercised"])
            self.assertFalse(receipt["claim_allowed"])
            self.assertTrue(Path(f"{receipt_path}.sha256").is_file())


if __name__ == "__main__":
    unittest.main()
