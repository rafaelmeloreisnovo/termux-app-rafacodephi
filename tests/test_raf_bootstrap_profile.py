from __future__ import annotations

import importlib.util
import json
from pathlib import Path
import tempfile
import unittest
import zipfile

MODULE_PATH = Path(__file__).resolve().parents[1] / "tools" / "raf_bootstrap_profile.py"
SPEC = importlib.util.spec_from_file_location("raf_bootstrap_profile", MODULE_PATH)
mod = importlib.util.module_from_spec(SPEC)
assert SPEC.loader is not None
SPEC.loader.exec_module(mod)


def write_zip(path: Path, entries: dict[str, bytes]) -> None:
    with zipfile.ZipFile(path, "w", compression=zipfile.ZIP_STORED) as zf:
        for name, data in entries.items():
            zf.writestr(name, data)


def base_bridge_entries() -> dict[str, bytes]:
    return {
        "BOOTSTRAP_INFO": (
            b"TERMUX_PACKAGE_NAME=com.termux.rafacodephi\n"
            b"TERMUX_ARCH=arm\n"
            b"BOOTSTRAP_FULLENGINE_READY=1\n"
        ),
        "SYMLINKS.txt": b"sh\xe2\x86\x90bin/raf-bootstrap-sh\n",
        "bin/sh": b"#!/system/bin/sh\nexit 0\n",
        "bin/pkg": b"#!/system/bin/sh\necho 'RAFCODEPHI pkg bridge'\n",
        "bin/apt": b"#!/system/bin/sh\necho 'RAFCODEPHI apt bridge'\n",
        "bin/apt-get": b"#!/system/bin/sh\necho 'RAFCODEPHI apt bridge'\n",
        "bin/busybox": b"#!/system/bin/sh\nexit 0\n",
        "bin/proot": b"#!/system/bin/sh\nexit 0\n",
    }


def real_entries() -> dict[str, bytes]:
    entries = base_bridge_entries()
    entries["bin/pkg"] = b"#!/system/bin/sh\nexec \"$PREFIX/bin/apt\" \"$@\"\n"
    entries["bin/apt"] = b"\x7fELF" + b"\x00" * 32
    entries["bin/apt-get"] = b"\x7fELF" + b"\x00" * 32
    entries["bin/dpkg"] = b"\x7fELF" + b"\x00" * 32
    entries["lib/libapt-pkg.so.7.0"] = b"\x7fELF" + b"\x00" * 32
    entries["etc/apt/sources.list"] = b"deb https://packages.example.invalid stable main\n"
    entries["var/lib/dpkg/status"] = b"\n"
    return entries


class BootstrapProfileTests(unittest.TestCase):
    def test_bridge_materialize_and_validate(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "bridge.zip"
            write_zip(path, base_bridge_entries())
            report = mod.materialize(
                path,
                profile="bridge",
                arch="arm",
                package_name="com.termux.rafacodephi",
                source_repo="local-test",
            )
            self.assertEqual("PASS", report["structural_state"])
            with zipfile.ZipFile(path) as zf:
                manifest = json.loads(zf.read("BOOTSTRAP_PROFILE.json"))
                info = mod.parse_info(zf.read("BOOTSTRAP_INFO"))
            self.assertEqual("bridge", manifest["profile"])
            self.assertFalse(manifest["claim_allowed"])
            self.assertEqual("0", info["BOOTSTRAP_FULLENGINE_READY"])

    def test_bridge_cannot_be_validated_as_real(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "bridge.zip"
            write_zip(path, base_bridge_entries())
            mod.materialize(path, profile="bridge", arch="arm",
                            package_name="com.termux.rafacodephi", source_repo="local-test")
            with self.assertRaises(mod.ProfileError):
                mod.validate(path, expected_profile="real-pkg", expected_arch="arm",
                             package_name="com.termux.rafacodephi")

    def test_real_profile_structural_candidate_passes(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "real.zip"
            write_zip(path, real_entries())
            report = mod.materialize(path, profile="real-pkg", arch="aarch64",
                                     package_name="com.termux.rafacodephi", source_repo="local-test")
            self.assertEqual("real-pkg", report["profile"])
            self.assertEqual("TOKEN_VAZIO", report["device_validation"])
            self.assertFalse(report["claim_allowed"])

    def test_real_profile_rejects_bridge_marker(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "bad.zip"
            entries = real_entries()
            entries["bin/apt"] = b"\x7fELFRAFCODEPHI apt bridge"
            write_zip(path, entries)
            with self.assertRaises(mod.ProfileError):
                mod.materialize(path, profile="real-pkg", arch="arm",
                                package_name="com.termux.rafacodephi", source_repo="local-test")

    def test_real_profile_rejects_legacy_prefix(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "legacy.zip"
            entries = real_entries()
            entries["lib/libapt-pkg.so.7.0"] += b"/data/data/com.termux/files/usr"
            write_zip(path, entries)
            with self.assertRaises(mod.ProfileError):
                mod.materialize(path, profile="real-pkg", arch="arm",
                                package_name="com.termux.rafacodephi", source_repo="local-test")

    def test_unsafe_zip_entry_is_rejected(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "unsafe.zip"
            entries = base_bridge_entries()
            entries["../escape"] = b"x"
            write_zip(path, entries)
            with self.assertRaises(mod.ProfileError):
                mod.materialize(path, profile="bridge", arch="arm",
                                package_name="com.termux.rafacodephi", source_repo="local-test")


if __name__ == "__main__":
    unittest.main()
