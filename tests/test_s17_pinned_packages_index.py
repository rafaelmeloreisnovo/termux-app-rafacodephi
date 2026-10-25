"""Hosted security regression: mutable Packages index must be pinned."""
import importlib.util
import hashlib
import os
from pathlib import Path
import unittest
from unittest.mock import patch

SCRIPT=Path(__file__).resolve().parents[1]/"scripts"/"build_real_arm_bootstrap_core.py"
spec=importlib.util.spec_from_file_location("s17real",SCRIPT)
core=importlib.util.module_from_spec(spec)
import sys
sys.modules[spec.name]=core
spec.loader.exec_module(core)
REPO="https://packages.termux.dev/apt/termux-main"
BODY=b"Package: apt\nVersion: 1\nFilename: apt_1.deb\nSHA256: "+b"a"*64+b"\n"
PIN=hashlib.sha256(BODY).hexdigest()
class S17Tests(unittest.TestCase):
 def test_missing_pin_blocked(self):
  with patch.dict(os.environ,{},clear=True):
   with self.assertRaisesRegex(SystemExit,"INDEPENDENT_PACKAGES_INDEX_SHA256_PIN_REQUIRED"):
    core.require_pinned_index(REPO,"arm")
 def test_wrong_scheme_blocked(self):
  with patch.dict(os.environ,{"RAFCODEPHI_INDEX_SHA256_ARM":PIN},clear=True):
   with self.assertRaisesRegex(SystemExit,"HTTPS_TRUSTED_REPO_REQUIRED"):
    core.require_pinned_index("http://packages.termux.dev/apt/termux-main","arm")
 def test_unapproved_host(self):
  with patch.dict(os.environ,{"RAFCODEPHI_INDEX_SHA256_ARM":PIN},clear=True):
   with self.assertRaisesRegex(SystemExit,"UNAPPROVED_REPOSITORY_HOST"):
    core.require_pinned_index("https://example.net/apt/termux-main","arm")
 def test_approved_host_and_pin(self):
  with patch.dict(os.environ,{"RAFCODEPHI_INDEX_SHA256_ARM":PIN},clear=True):
   self.assertEqual(core.require_pinned_index(REPO,"arm"),PIN)
 def test_matching_index_bytes(self):
  with patch.dict(os.environ,{"RAFCODEPHI_INDEX_SHA256_ARM":PIN},clear=True):
   with patch.object(core,"fetch_bytes",return_value=BODY) as fetch:
    self.assertEqual(core.fetch_pinned_index(REPO,"arm"),BODY)
    self.assertEqual(fetch.call_count,1)
 def test_modified_index_blocks(self):
  with patch.dict(os.environ,{"RAFCODEPHI_INDEX_SHA256_ARM":PIN},clear=True):
   with patch.object(core,"fetch_bytes",return_value=BODY+b"tamper"):
    with self.assertRaisesRegex(SystemExit,"PACKAGES_INDEX_DIGEST_MISMATCH"):
     core.fetch_pinned_index(REPO,"arm")
 def test_unsigned_untrusted_pin_never_claims_verified_signature(self):
  self.assertNotIn("gpgv",core.require_pinned_index.__code__.co_names)
  self.assertNotIn("gpgv",core.fetch_pinned_index.__code__.co_names)
 def test_unsupported_arch(self):
  with patch.dict(os.environ,{"RAFCODEPHI_INDEX_SHA256_ARM":PIN},clear=True):
   with self.assertRaisesRegex(SystemExit,"UNSUPPORTED_INDEX_ARCH"):
    core.require_pinned_index(REPO,"x86_64")
if __name__=="__main__":unittest.main()
