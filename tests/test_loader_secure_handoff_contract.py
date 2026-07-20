from __future__ import annotations

import importlib.util
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    "loader_handoff", ROOT / "tools" / "validate_loader_secure_handoff.py")
assert SPEC and SPEC.loader
module = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(module)

def fixture() -> dict[str, str]: return module.load(ROOT)

class LoaderSecureHandoffTests(unittest.TestCase):
    def invalid(self, files: dict[str, str], text: str) -> None:
        errors = module.validate(files)
        self.assertTrue(errors); self.assertIn(text, "\n".join(errors))

    def test_valid(self): self.assertEqual([], module.validate(fixture()))
    def test_target_dir_rejected(self):
        f=fixture(); f["contract"] += ' EXTRA_TARGET_DIR="target_dir"'; self.invalid(f,"target directory")
    def test_extraction_rejected(self):
        f=fixture(); f["service"] += " ZipInputStream"; self.invalid(f,"extracts")
    def test_signature_required(self):
        f=fixture(); f["host_manifest"]=f["host_manifest"].replace('protectionLevel="signature"','protectionLevel="dangerous"'); self.invalid(f,"signature custody")
    def test_cross_origin_policy_required(self):
        f=fixture(); f["policy"]=f["policy"].replace("CROSS_ORIGIN_REDIRECT_BLOCKED","ALLOW_REDIRECT"); self.invalid(f,"same-origin")
    def test_implicit_redirect_rejected(self):
        f=fixture(); f["service"]=f["service"].replace("setInstanceFollowRedirects(false)","setInstanceFollowRedirects(true)"); self.invalid(f,"bounded acquisition")
    def test_provider_readonly_required(self):
        f=fixture(); f["provider"]=f["provider"].replace("MODE_READ_ONLY","MODE_READ_WRITE"); self.invalid(f,"provider")
    def test_blake3_required(self):
        f=fixture(); f["receiver"]=f["receiver"].replace("BootstrapIntegrityVerifier.blake3Hex","removed"); self.invalid(f,"double-hash")
    def test_zip_budget_required(self):
        f=fixture(); f["receiver"]=f["receiver"].replace("MAX_UNCOMPRESSED_BYTES","REMOVED"); self.invalid(f,"ZIP custody")
    def test_native_malloc_rejected(self):
        f=fixture(); f["native"] += " malloc(1);"; self.invalid(f,"native heap")
    def test_pin_defaults_empty(self):
        f=fixture(); f["pin_gradle"]=f["pin_gradle"].replace('System.getenv("TERMUX_EXTERNAL_BOOTSTRAP_URL_AARCH64") ?: ""','System.getenv("TERMUX_EXTERNAL_BOOTSTRAP_URL_AARCH64") ?: "https://invented.invalid"'); self.invalid(f,"must default empty")
    def test_signing_boundary_required(self):
        f=fixture(); f["loader_gradle"]=f["loader_gradle"].replace("TERMUX_RELEASE_KEYSTORE_FILE","REMOVED"); self.invalid(f,"signer boundary")
    def test_launcher_gate_required(self):
        f=fixture(); f["gate"]=f["gate"].replace("BootstrapLoaderClient.requestIfConfigured","bypass"); self.invalid(f,"launcher gate")
    def test_root_applies_pin_contract(self):
        f=fixture(); f["root_gradle"]=""; self.invalid(f,"does not apply")

    def test_source_policy_executes(self):
        javac, java = shutil.which("javac"), shutil.which("java")
        if not javac or not java: self.skipTest("JDK unavailable")
        source=ROOT/module.FILES["policy"]
        harness='''package com.termux.rafacodephi.loader;
public final class BootstrapSourcePolicyHarness {
 interface C { void run() throws Exception; }
 static void reject(C c) throws Exception { boolean ok=false; try{c.run();}catch(Exception e){ok=true;} if(!ok)throw new AssertionError(); }
 public static void main(String[] a) throws Exception {
  java.net.URL origin=BootstrapSourcePolicy.requireInitialUrl("https://example.com/a.zip");
  BootstrapSourcePolicy.requireSameOriginRedirect(origin,origin,"/b.zip");
  BootstrapSourcePolicy.requireAbi("aarch64"); BootstrapSourcePolicy.requireSha256("a".repeat(64));
  reject(()->BootstrapSourcePolicy.requireInitialUrl("http://example.com/a"));
  reject(()->BootstrapSourcePolicy.requireInitialUrl("https://u:p@example.com/a"));
  reject(()->BootstrapSourcePolicy.requireInitialUrl("https://example.com:444/a"));
  reject(()->BootstrapSourcePolicy.requireSameOriginRedirect(origin,origin,"https://other.example/b"));
  reject(()->BootstrapSourcePolicy.requireAbi("mips")); reject(()->BootstrapSourcePolicy.requireSha256("00"));
 }}'''
        with tempfile.TemporaryDirectory() as td:
            p=Path(td)/"BootstrapSourcePolicyHarness.java"; p.write_text(harness)
            subprocess.run([javac,"-source","11","-target","11","-d",td,str(source),str(p)],check=True,capture_output=True,text=True)
            subprocess.run([java,"-cp",td,"com.termux.rafacodephi.loader.BootstrapSourcePolicyHarness"],check=True,capture_output=True,text=True)

if __name__ == "__main__": unittest.main()
