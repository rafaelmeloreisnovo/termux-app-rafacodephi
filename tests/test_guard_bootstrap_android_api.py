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
        "  Android 0x00000084 NT_VERSION (version)\n"
        "   description data: "
        + " ".join(f"{x:02x}" for x in api.to_bytes(4, "little")) + " \n"
    )
    symbols = "  11: 00000000 0 FUNC GLOBAL DEFAULT UND read@LIBC (2)\n"
    if bad_import:
        symbols += "  18: 00000000 0 FUNC GLOBAL DEFAULT UND memfd_create@LIBC_R (7)\n"
    return {"-h": f"  Machine:                           {machine}\n",
            "-n": notes, "--dyn-syms": symbols}


class ApiGateTests(unittest.TestCase):
    def inspect(self, api=29, machine="ARM", bad_import=False,
                missing_note=False, elf=True, arch="arm", max_api=29):
        outputs = tool_outputs(api, machine, bad_import, missing_note)
        binary = b"\x7fELF" + bytes([gate.ARCH[arch][1]]) + b"\0" * 100 if elf else b"TEXT"
        with patch.object(gate, "readelf", side_effect=lambda _data, flag: outputs[flag]):
            return gate.inspect_elf(binary, arch, "bin/bash", max_api)

    def test_api29_without_unresolved_memfd_passes_static_scope(self):
        self.assertEqual(self.inspect()["state"], "PASS_SCOPED_STATIC")

    def test_api30_is_blocked_on_android10(self):
        self.assertIn("ANDROID_MIN_API_EXCEEDS_TARGET", self.inspect(api=30)["reasons"])

    def test_arm64_api30_memfd_import_is_valid_for_android11(self):
        result = self.inspect(api=30, machine="AArch64", bad_import=True,
                              arch="aarch64", max_api=30)
        self.assertEqual(result["state"], "PASS_SCOPED_STATIC")

    def test_arm32_api30_remains_blocked_for_android10(self):
        result = self.inspect(api=30, arch="arm", max_api=29)
        self.assertIn("ANDROID_MIN_API_EXCEEDS_TARGET", result["reasons"])

    def test_api29_memfd_is_rejected_even_with_higher_target(self):
        result = self.inspect(api=29, machine="AArch64", bad_import=True,
                              arch="aarch64", max_api=30)
        self.assertIn("UNRESOLVED_MEMFD_CREATE", result["reasons"])

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
        self.assertEqual(len(result["checks"]), len(gate.CRITICAL) + len(gate.SCRIPT_INTERPRETERS))
        self.assertIn("NOT_ELF", result["checks"][0]["reasons"])

    def test_android10_rejects_transitive_libcrypto_memfd_import(self):
        # A valid bin/bash does not prove its linked .so closure can load.
        with tempfile.TemporaryDirectory() as d:
            archive = Path(d) / "bootstrap.zip"
            with zipfile.ZipFile(archive, "w") as z:
                z.writestr("BOOTSTRAP_PROFILE.json",
                           json.dumps({"arch": "arm",
                                       "package_name": "com.termux.rafacodephi"}))
                for name in gate.CRITICAL:
                    z.writestr(name, b"\x7fELF" + b"\x01" * 64)
                for name, interpreter in gate.SCRIPT_INTERPRETERS.items():
                    z.writestr(name, b"#!" + interpreter + b"\n")
                z.writestr("lib/libcrypto.so.3", b"\x7fELF" + b"\x01" * 64)
            def probe(_binary, arch, name, max_api):
                reasons = ["UNRESOLVED_MEMFD_CREATE"] if name == "lib/libcrypto.so.3" else []
                return {"entry": name, "arch": arch,
                        "state": "BLOCKED" if reasons else "PASS_SCOPED_STATIC",
                        "android_min_api": 29, "reasons": reasons}
            with patch.object(gate, "inspect_elf", side_effect=probe):
                report = gate.inspect_archive("arm", archive, 29)
        self.assertEqual(report["state"], "BLOCKED")
        blocked = {x["entry"]: x["reasons"] for x in report["checks"] if x["reasons"]}
        self.assertEqual(blocked["lib/libcrypto.so.3"], ["UNRESOLVED_MEMFD_CREATE"])

    def test_pkg_shebang_references_private_bash(self):
        valid = b"#!/data/data/com.termux.rafacodephi/files/usr/bin/bash\nset -eu\n"
        self.assertEqual(gate.inspect_script(valid, "arm", "bin/pkg")["state"],
                         "PASS_SCOPED_STATIC")

    def test_pkg_shebang_wrong_package_prefix_is_blocked(self):
        stale = b"#!/data/data/com.termux/files/usr/bin/bash\n"
        self.assertIn("SHEBANG_PREFIX_MISMATCH",
                      gate.inspect_script(stale, "arm", "bin/pkg")["reasons"])

    def test_wrong_arch_profile_is_blocked(self):
        with tempfile.TemporaryDirectory() as d:
            p = Path(d) / "bootstrap.zip"
            with zipfile.ZipFile(p, "w") as z:
                z.writestr("BOOTSTRAP_PROFILE.json",
                           json.dumps({"arch": "aarch64",
                                       "package_name": "com.termux.rafacodephi"}))
            result = gate.inspect_archive("arm", p, 29)
        self.assertIn("ARCH_PROFILE_MISMATCH", result["reasons"])

    def test_profile_must_be_json_object_and_yield_typed_blocked(self):
        with tempfile.TemporaryDirectory() as d:
            p = Path(d) / "not-object.zip"
            with zipfile.ZipFile(p, "w") as z:
                z.writestr("BOOTSTRAP_PROFILE.json", "[]")
            result = gate.inspect_archive("arm", p, 29)
        self.assertEqual(result["state"], "BLOCKED")
        self.assertIn("PROFILE_NOT_OBJECT", result["reasons"])

    def test_profile_invalid_utf8_is_typed_blocked(self):
        with tempfile.TemporaryDirectory() as d:
            p = Path(d) / "invalid-encoding.zip"
            with zipfile.ZipFile(p, "w") as z:
                z.writestr("BOOTSTRAP_PROFILE.json", b"\xff")
            result = gate.inspect_archive("arm", p, 29)
        self.assertEqual(result["state"], "BLOCKED")
        self.assertIn("PROFILE_INVALID_ENCODING", result["reasons"])

    def test_profile_size_limit_blocks_before_read(self):
        with tempfile.TemporaryDirectory() as d:
            p = Path(d) / "large-profile.zip"
            with zipfile.ZipFile(p, "w", compression=zipfile.ZIP_DEFLATED) as z:
                z.writestr("BOOTSTRAP_PROFILE.json", b" " * (64 * 1024 + 1))
            result = gate.inspect_archive("arm", p, 29)
        self.assertEqual(result["state"], "BLOCKED")
        self.assertIn("PROFILE_SIZE_LIMIT", result["reasons"])

    def test_profile_duplicate_keys_are_rejected(self):
        with tempfile.TemporaryDirectory() as d:
            p = Path(d) / "duplicate-profile-key.zip"
            with zipfile.ZipFile(p, "w") as z:
                z.writestr("BOOTSTRAP_PROFILE.json",
                           '{"arch":"aarch64","arch":"arm",'
                           '"package_name":"com.termux.rafacodephi"}')
            result = gate.inspect_archive("arm", p, 29)
        self.assertEqual(result["state"], "BLOCKED")
        self.assertIn("PROFILE_DUPLICATE_JSON_KEY", result["reasons"])

    def test_profile_stream_limit_even_if_metadata_is_inaccurate(self):
        with tempfile.TemporaryDirectory() as d:
            p = Path(d) / "misreported-profile.zip"
            with zipfile.ZipFile(p, "w") as z:
                z.writestr("BOOTSTRAP_PROFILE.json", "{}")
            original_open = zipfile.ZipFile.open

            def simulated_misreported_member(archive, name, *args, **kwargs):
                if getattr(name, "filename", name) == "BOOTSTRAP_PROFILE.json":
                    return io.BytesIO(b" " * (gate.PROFILE_MAX_BYTES + 1))
                return original_open(archive, name, *args, **kwargs)

            with patch.object(zipfile.ZipFile, "open",
                              new=simulated_misreported_member):
                result = gate.inspect_archive("arm", p, 29)
        self.assertEqual(result["state"], "BLOCKED")
        self.assertIn("PROFILE_SIZE_LIMIT", result["reasons"])

    def test_unsupported_zip_method_yields_blocked_receipt(self):
        with tempfile.TemporaryDirectory() as d:
            p = Path(d) / "unsupported-compression.zip"
            with zipfile.ZipFile(p, "w") as z:
                z.writestr("BOOTSTRAP_PROFILE.json", "{}")
            with patch.object(zipfile.ZipFile, "open",
                              side_effect=NotImplementedError("UNSUPPORTED_ZIP_METHOD")):
                result = gate.inspect_archive("arm", p, 29)
        self.assertEqual(result["state"], "BLOCKED")
        self.assertIn("UNSUPPORTED_ZIP_METHOD", result["reasons"])

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

    def test_cli_abi_lane_does_not_require_other_archive(self):
        with tempfile.TemporaryDirectory() as d:
            base = Path(d)
            out = base / "one-lane.json"
            state = gate.main(["--arm", str(base / "not-arm.zip"),
                               "--aarch64", str(base / "not-aarch64.zip"),
                               "--only", "aarch64", "--max-api", "30",
                               "--out", str(out)])
            doc = json.loads(out.read_text())
        self.assertEqual(state, 1)
        self.assertEqual(len(doc["archives"]), 1)
        self.assertEqual(doc["archives"][0]["arch"], "aarch64")
        self.assertEqual(doc["device_api_target"], 30)
        self.assertFalse(doc["claim_allowed"])

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
