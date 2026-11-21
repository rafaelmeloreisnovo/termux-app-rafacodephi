"""Host-side execution checks for the shell collector's literal Python receipt emitter."""
from __future__ import annotations

import importlib.util
import json
import subprocess
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
COLLECTOR = ROOT / "scripts/collect_e2e_device_receipt.sh"
SPEC = importlib.util.spec_from_file_location(
    "validate_e2e_receipt", ROOT / "scripts/validate_e2e_receipt.py")
VALIDATOR = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(VALIDATOR)


def emitter_source() -> str:
    text = COLLECTOR.read_text(encoding="utf-8")
    anchor = '  "$WORKLOAD" "$workload_rc" "$stdout_sha256" <<\'PY\'\n'
    if text.count(anchor) != 1:
        raise AssertionError("collector must own exactly one literal receipt emitter")
    start = text.index(anchor) + len(anchor)
    end = text.index("\nPY\n", start)
    code = text[start:end] + "\n"
    if "${" in code:
        raise AssertionError("no shell interpolation in Python source")
    return code


class PhysicalCollectorEmissionTests(unittest.TestCase):
    def test_malicious_workload_is_data_not_executable_code(self) -> None:
        with tempfile.TemporaryDirectory() as td:
            output = Path(td) / "receipt.json"
            marker = Path(td) / "must-not-be-created"
            payload = "hello'; __import__('pathlib').Path(" + repr(str(marker)) + ").write_text('x'); #\n{\"z\": 3}"
            args = [
                str(output), "2026-11-21T09:00:00-03:00",
                "rafaelmeloreisnovo/termux-app-rafacodephi", "a" * 40,
                "b" * 64, "c" * 64,
                "rafaelmeloreisnovo/termux-packages", "d" * 40,
                "e" * 64, "f" * 64, "1" * 64,
                "manufacturer'quote", "model\nnewline", "10", "armeabi-v7a",
                "com.termux.rafacodephi",
                "/data/data/com.termux.rafacodephi/files/usr",
                "/data/app/fixture/base.apk", "c" * 64,
                payload, "0", "2" * 64,
            ]
            run = subprocess.run(
                ["python3", "-", *args], input=emitter_source(), text=True,
                capture_output=True, timeout=15,
            )
            self.assertEqual(run.returncode, 0, run.stderr)
            self.assertFalse(marker.exists(), "workload strings must not execute as Python")
            receipt = json.loads(output.read_text(encoding="utf-8"))
            self.assertEqual(receipt["workload"]["command"], payload)
            self.assertEqual(receipt["device"]["model"], "model\nnewline")
            self.assertFalse(receipt["claim_allowed"])
            self.assertEqual(receipt["stages"]["reproduction"], "TOKEN_VAZIO")
            self.assertEqual(VALIDATOR.validate_structure(receipt), [])

    def test_receipts_are_private_append_only(self) -> None:
        text = COLLECTOR.read_text(encoding="utf-8")
        self.assertIn("umask 077", text)
        self.assertIn('receipt_tmp="$(mktemp "$receipt_dir/.e2e-receipt.XXXXXXXX")"', text)
        self.assertIn('chmod 600 "$receipt_tmp"', text)
        self.assertIn('ln -- "$receipt_tmp" "$OUT"', text)
        self.assertNotIn('cp "$tmp_json" "$OUT"', text)

    def test_collector_rejects_non_literal_heredoc_regression(self) -> None:
        text = COLLECTOR.read_text(encoding="utf-8")
        self.assertNotIn("${WORKLOAD@Q}", text)
        self.assertNotIn("<<PY\n", text)
        self.assertIn("<<'PY'\n", text)


if __name__ == "__main__":
    unittest.main()
