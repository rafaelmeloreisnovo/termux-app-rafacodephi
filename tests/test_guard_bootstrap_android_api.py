#!/usr/bin/env python3
"""Adversarial, host-only unit tests. No Android execution or source builds."""
import importlib.util
import io
import json
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch
import zipfile

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location(
    "raf_api_gate", ROOT / "scripts/guard_bootstrap_android_api.py"
)
gate = importlib.util.module_from_spec(spec)
spec.loader.exec_module(gate)


def tool_outputs(api=29, machine="ARM", bad_import=False, missing_note=False):
    notes = "" if missing_note else (
        "Displaying notes found in: .note.android.ident\n"
        "  Android 0x00000084 NT_VERSION (version) description data: "
        + " ".join(f"{x:02x}" for x in api.to_bytes(4, "little")) + " \n"
    )
    symbols = "  11: 00000000 0 FUNC GLOBAL DEFAULT UND read@LIBC (2)\n"
    if bad_import:
        symbols += "  18: 00000000 0 FUNC GLOBAL DEFAULT UND memfd_create@LIBC_R (7)\n"
    return {"-h": f"  Machine:                           {machine}\n",
            "-n": notes, "--dyn-syms": symbols}


class ApiGateTests(unittest.TestCase):
    def inspect(self, api=29, machine="ARM", bad_import=False,
                missing_note=False, elf=True, arch="arm"):
        outputs = tool_outputs(api, machine, bad_import, missing_note)
        binary = b"\x7fELF" + bytes([gate.ARCH[arch][1]]) + b"\0" * 100 if elf else b"TEXT"
        with patch.object(gate, "readelf", side_effect=lambda _data, flag: outputs[flag]):
            return gate.inspect_elf(binary, arch, "bin/bash", 29)

    def test_api29_without_unresolved_memfd_passes_static_scope(self):
        self.assertEqual(self.inspect()["state"], "PASS_SCOPED_STATIC")

    def test_api30_is_blocked_on_android10(self):
        self.assertIn("ANDROID_MIN_API_EXCEEDS_TARGET", self.inspect(api=30)["reasons"])

    def test_api29_with_memfd_import_is_blocked(self):
        self.assertIn("UNRESOLVED_MEMFD_CREATE",
                      self.inspect(bad_import=True)["reasons"])

    def test_android_api_note_missing_is_blocked(self):
        self.assertIn("ANDROID_API_NOTE_MISSING",
                      self.inspect(missing_note=True)["reasons"])

    def test_elf_machine_mismatch_is_blocked(self):
        self.assertIn("ELF_MACHINE_MISMATCH",
                      self.inspect(machine="AArch64")["reasons"])

    def test_non_elf_is_blocked(self):
        self.assertIn("NOT_ELF", self.inspect(elf=False)["reasons"])

    def test_missing_archive_is_not_pass(self):
        with tempfile.TemporaryDirectory() as d:
            report = gate.inspect_archive("arm", Path(d) / "absent.zip", 29)
        self.assertEqual(report["state"], "BLOCKED")
        self.assertIn("BOOTSTRAP_ARCHIVE_MISSING", report["reasons"])

    def test_archive_requires_every_critical_executable(self):
        with tempfile.TemporaryDirectory() as d:
            p = Path(d) / "bootstrap.zip"
            with zipfile.ZipFile(p, "w") as z:
                z.writestr("BOOTSTRAP_PROFILE.json",
                           json.dumps({"arch": "arm", "package_name": "com.termux.rafacodephi"}))
                z.writestr("bin/bash", b"not-an-elf")
            result = gate.inspect_archive("arm", p, 29)
        self.assertEqual(result["state"], "BLOCKED")
        self.assertEqual(len(result["checks"]), len(gate.CRITICAL))
        self.assertIn("NOT_ELF", result["checks"][0]["reasons"])

    def test_wrong_arch_profile_is_blocked(self):
        with tempfile.TemporaryDirectory() as d:
            p = Path(d) / "bootstrap.zip"
            with zipfile.ZipFile(p, "w") as z:
                z.writestr("BOOTSTRAP_PROFILE.json",
                           json.dumps({"arch": "aarch64",
                                       "package_name": "com.termux.rafacodephi"}))
            result = gate.inspect_archive("arm", p, 29)
        self.assertIn("ARCH_PROFILE_MISMATCH", result["reasons"])

    def test_duplicate_zip_names_block_before_inspection(self):
        import warnings
        with tempfile.TemporaryDirectory() as d:
            p = Path(d) / "bootstrap.zip"
            with warnings.catch_warnings():
                warnings.simplefilter("ignore", UserWarning)
                with zipfile.ZipFile(p, "w") as z:
                    z.writestr("BOOTSTRAP_PROFILE.json", "{}")
                    z.writestr("BOOTSTRAP_PROFILE.json", "{}")
            result = gate.inspect_archive("arm", p, 29)
        self.assertIn("DUPLICATE_ARCHIVE_ENTRIES", result["reasons"])

    def test_cli_writes_blocked_receipt_even_when_inputs_missing(self):
        with tempfile.TemporaryDirectory() as d:
            base = Path(d)
            out = base / "receipt.json"
            state = gate.main(["--arm", str(base / "not-arm.zip"),
                               "--aarch64", str(base / "not-64.zip"),
                               "--out", str(out)])
            receipt = json.loads(out.read_text())
        self.assertEqual(state, 1)
        self.assertEqual(receipt["state"], "BLOCKED")
        self.assertEqual(receipt["device_execution"], "TOKEN_VAZIO")
        self.assertFalse(receipt["claim_allowed"])


if __name__ == "__main__":
    unittest.main()
