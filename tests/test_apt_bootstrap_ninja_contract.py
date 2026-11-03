"""Synthetic producer/consumer handoff, never physical Android evidence."""
from __future__ import annotations

import importlib.util
import json
from pathlib import Path
import tempfile
import unittest
import zipfile

ROOT = Path(__file__).resolve().parents[1]


def module_at(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


IMPORTER = module_at("ninja_import", ROOT / "scripts/import_rafcodephi_real_bootstrap.py")
FIXTURE = module_at("ninja_fixture", ROOT / "tests/test_source_built_real_bootstrap.py")
CONTRACT = "rafcodephi.apt-bootstrap-ninja/v1"


class AptBootstrapNinjaTests(unittest.TestCase):
    def validate(self, arch="arm", *, mutation=None, legacy=False):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            archive, manifest = root / "bootstrap.zip", root / "manifest.txt"
            FIXTURE.make_zip(archive, arch=arch)
            with zipfile.ZipFile(archive) as zf:
                entries = {i.filename: (i, zf.read(i)) for i in zf.infolist()}
            profile = json.loads(entries["BOOTSTRAP_PROFILE.json"][1])
            info = entries["BOOTSTRAP_INFO"][1]
            if not legacy:
                profile["build_tools_contract"] = CONTRACT
                profile["required_entries"].append("bin/ninja")
                info += f"RAFCODEPHI_BUILD_TOOLS_CONTRACT={CONTRACT}\n".encode()
                entries["bin/ninja"] = ("bin/ninja", FIXTURE.elf_for_arch(arch))
                status = entries["var/lib/dpkg/status"][1]
                entries["var/lib/dpkg/status"] = ("var/lib/dpkg/status", status +
                    b"\n\nPackage: ninja\nStatus: install ok installed\nVersion: 1.13.2\n\n")
            if mutation == "profile":
                profile["build_tools_contract"] = CONTRACT + "-unsupported"
            if mutation == "info":
                info = info.replace(f"RAFCODEPHI_BUILD_TOOLS_CONTRACT={CONTRACT}\n".encode(), b"")
            if mutation == "missing":
                entries.pop("bin/ninja")
            if mutation == "script":
                entries["bin/ninja"] = ("bin/ninja", b"#!/system/bin/sh\nexit 0\n")
            if mutation == "wrong_arch":
                entries["bin/ninja"] = ("bin/ninja", FIXTURE.elf_for_arch("aarch64" if arch == "arm" else "arm"))
            if mutation == "legacy_prefix":
                entries["bin/ninja"] = ("bin/ninja", FIXTURE.elf_for_arch(arch, IMPORTER.LEGACY_PREFIX))
            if mutation == "not_installed":
                name, status = entries["var/lib/dpkg/status"]
                entries["var/lib/dpkg/status"] = (name, status.replace(
                    b"Package: ninja\nStatus: install ok installed", b"Package: ninja\nStatus: deinstall ok config-files"))
            if mutation == "undeclared_entry":
                profile["required_entries"].remove("bin/ninja")
            entries["BOOTSTRAP_PROFILE.json"] = ("BOOTSTRAP_PROFILE.json", json.dumps(profile).encode())
            entries["BOOTSTRAP_INFO"] = ("BOOTSTRAP_INFO", info)
            with zipfile.ZipFile(archive, "w") as zf:
                for name, payload in entries.values():
                    zf.writestr(name, payload)
            FIXTURE.make_manifest(manifest, archive, arch=arch)
            if not legacy:
                with manifest.open("a") as out:
                    out.write(f"build_tools_contract={CONTRACT}\nbootstrap_ninja=EMBEDDED\n")
            if mutation == "manifest":
                manifest.write_text(manifest.read_text().replace("bootstrap_ninja=EMBEDDED", "bootstrap_ninja=TOKEN_VAZIO"))
            return IMPORTER.validate(archive, manifest, arch)

    def test_dual_abi_handoff(self):
        for arch in ("arm", "aarch64"):
            with self.subTest(arch=arch):
                result = self.validate(arch)
                self.assertEqual(result["build_tools_contract"], CONTRACT)
                self.assertFalse(result["claim_allowed_device_runtime"])

    def test_legacy_does_not_claim_ninja(self):
        self.assertEqual(self.validate(legacy=True)["build_tools_contract"], "LEGACY_NOT_DECLARED")

    def test_negative_handoffs(self):
        for mutation in ("profile", "info", "manifest", "missing", "script", "wrong_arch",
                         "legacy_prefix", "not_installed", "undeclared_entry"):
            with self.subTest(mutation=mutation), self.assertRaises(SystemExit):
                self.validate(mutation=mutation)


if __name__ == "__main__":
    unittest.main()
