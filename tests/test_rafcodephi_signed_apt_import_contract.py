#!/usr/bin/env python3
"""Host-only negative/positive tests of signed APT trust handoff. No Android claim."""
from __future__ import annotations
import hashlib
import importlib.util
import io
import os
import sys
import unittest
import zipfile
from pathlib import Path
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    "rafcodephi_apt_import", ROOT / "scripts/import_rafcodephi_real_bootstrap.py")
assert SPEC and SPEC.loader
MODULE = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = MODULE
SPEC.loader.exec_module(MODULE)

URL = "https://example.invalid/rafcodephi/repo"
KEY = b"public-key-fixture-not-a-production-signing-key"
PIN = hashlib.sha256(KEY).hexdigest()
PREFIX = "/data/data/com.termux.rafacodephi/files/usr"


def fixture(*, url=URL, active_guard=False, trusted_yes=False, correct_key=True):
    manifest = {
        "package_repo_runtime_state": "SIGNED_REPOSITORY_CONFIGURED",
        "apt_repository_url": url,
        "apt_repository_trust": "SIGNED_BY_ARCHIVE_KEY",
    }
    profile = {
        "package_repo_runtime_state": "SIGNED_REPOSITORY_CONFIGURED",
        "apt_repository_url": url,
        "apt_repository_trust": "SIGNED_BY_ARCHIVE_KEY",
        "apt_update_guard": "DISABLED_SIGNED_REPOSITORY_CONFIGURED",
    }
    info = {
        "RAFCODEPHI_APT_REPOSITORY_URL": url,
        "RAFCODEPHI_APT_REPOSITORY_TRUST": "SIGNED_BY_ARCHIVE_KEY",
        "RAFCODEPHI_APT_UPDATE_GUARD": "DISABLED_SIGNED_REPOSITORY_CONFIGURED",
    }
    source = ("# RAFCODEPHI_PACKAGE_REPOSITORY=SIGNED_REPOSITORY_CONFIGURED\n"
              "Enabled: yes\nTypes: deb\n"
              f"URIs: {url.rstrip('/')}\nSuites: stable\nComponents: main\n"
              "Architectures: arm aarch64\n"
              f"Signed-By: {PREFIX}/etc/apt/keyrings/rafcodephi-archive-key.gpg\n"
              + ("Trusted: yes\n" if trusted_yes else ""))
    buf = io.BytesIO()
    with zipfile.ZipFile(buf, "w") as archive:
        archive.writestr(MODULE.APT_SOURCE_PATH, source)
        archive.writestr(MODULE.APT_BLOCK_PATH,
                         'APT::Update::Pre-Invoke { "exit 100"; };\n'
                         if active_guard else "// APT blocker inactive\n")
        archive.writestr(MODULE.APT_KEY_PATH, KEY if correct_key else b"wrong-public-key")
    buf.seek(0)
    return manifest, profile, info, buf


class SignedAptContractTests(unittest.TestCase):
    def check_contract(self, **kw):
        manifest, profile, info, data = fixture(**kw)
        with zipfile.ZipFile(data) as zf:
            MODULE.validate_signed_apt_contract(manifest, profile, info, zf)

    def test_good_pinned_key(self):
        with patch.dict(os.environ, {"RAF_APT_TRUSTED_KEY_SHA256": PIN}):
            self.check_contract()

    def test_missing_independent_trust_anchor_fails(self):
        with patch.dict(os.environ, {"RAF_APT_TRUSTED_KEY_SHA256": ""}):
            with self.assertRaisesRegex(SystemExit, "independent"):
                self.check_contract()

    def test_wrong_key_fails(self):
        with patch.dict(os.environ, {"RAF_APT_TRUSTED_KEY_SHA256": PIN}):
            with self.assertRaisesRegex(SystemExit, "differs"):
                self.check_contract(correct_key=False)

    def test_active_blocker_fails(self):
        with patch.dict(os.environ, {"RAF_APT_TRUSTED_KEY_SHA256": PIN}):
            with self.assertRaisesRegex(SystemExit, "blocking"):
                self.check_contract(active_guard=True)

    def test_trusted_yes_bypass_fails(self):
        with patch.dict(os.environ, {"RAF_APT_TRUSTED_KEY_SHA256": PIN}):
            with self.assertRaisesRegex(SystemExit, "fields"):
                self.check_contract(trusted_yes=True)

    def test_insecure_http_fails(self):
        with patch.dict(os.environ, {"RAF_APT_TRUSTED_KEY_SHA256": PIN}):
            with self.assertRaisesRegex(SystemExit, "canonical HTTPS"):
                self.check_contract(url="http://example.invalid/rafcodephi")

    def test_invalid_userinfo_fails(self):
        with patch.dict(os.environ, {"RAF_APT_TRUSTED_KEY_SHA256": PIN}):
            with self.assertRaisesRegex(SystemExit, "canonical HTTPS"):
                self.check_contract(url="https://a:b@example.invalid/rafcodephi")

    def test_injected_source_field_fails(self):
        with patch.dict(os.environ, {"RAF_APT_TRUSTED_KEY_SHA256": PIN}):
            with self.assertRaisesRegex(SystemExit, "canonical HTTPS"):
                self.check_contract(url="https://example.invalid/r\nTrusted: yes")


if __name__ == "__main__":
    unittest.main()
