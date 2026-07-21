from __future__ import annotations

import importlib.util
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
VALIDATOR = ROOT / "tools" / "validate_loader_secure_handoff.py"
SPEC = importlib.util.spec_from_file_location("loader_handoff", VALIDATOR)
assert SPEC and SPEC.loader
module = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(module)


def fixture() -> dict[str, str]:
    return module.load(ROOT)


class LoaderSecureHandoffContractTests(unittest.TestCase):

    def invalid(self, files: dict[str, str], text: str) -> None:
        errors = module.validate(files)
        self.assertTrue(errors)
        self.assertIn(text, "\n".join(errors))

    def test_canonical_contract_valid(self) -> None:
        self.assertEqual([], module.validate(fixture()))

    def test_target_directory_rejected(self) -> None:
        files = fixture()
        files["contract"] += '\nString EXTRA_TARGET_DIR = "target_dir";\n'
        self.invalid(files, "target directory")

    def test_loader_extraction_rejected(self) -> None:
        files = fixture()
        files["service"] += "\nZipInputStream forbidden;\n"
        self.invalid(files, "extracts")

    def test_signature_permission_required(self) -> None:
        files = fixture()
        files["host_manifest"] = files["host_manifest"].replace(
            'android:protectionLevel="signature"',
            'android:protectionLevel="dangerous"',
        )
        self.invalid(files, "signature custody")

    def test_cross_origin_redirect_policy_required(self) -> None:
        files = fixture()
        files["policy"] = files["policy"].replace(
            "CROSS_ORIGIN_REDIRECT_BLOCKED",
            "REDIRECT_ALLOWED",
        )
        self.invalid(files, "same-origin")

    def test_loader_must_not_follow_redirects_implicitly(self) -> None:
        files = fixture()
        files["service"] = files["service"].replace(
            "setInstanceFollowRedirects(false)",
            "setInstanceFollowRedirects(true)",
        )
        self.invalid(files, "bounded download")

    def test_read_only_provider_required(self) -> None:
        files = fixture()
        files["provider"] = files["provider"].replace(
            "ParcelFileDescriptor.MODE_READ_ONLY",
            "ParcelFileDescriptor.MODE_READ_WRITE",
        )
        self.invalid(files, "read-only")

    def test_host_blake3_required(self) -> None:
        files = fixture()
        files["receiver"] = files["receiver"].replace(
            "BootstrapIntegrityVerifier.blake3Hex",
            "removedBlake3",
        )
        self.invalid(files, "double hash")

    def test_zip_budget_required(self) -> None:
        files = fixture()
        files["receiver"] = files["receiver"].replace(
            "MAX_UNCOMPRESSED_BYTES",
            "REMOVED_UNCOMPRESSED_LIMIT",
        )
        self.invalid(files, "ZIP budgets")

    def test_native_malloc_rejected(self) -> None:
        files = fixture()
        files["native"] += "\nvoid *forbidden = malloc(1);\n"
        self.invalid(files, "native heap")

    def test_external_pin_defaults_must_be_empty(self) -> None:
        files = fixture()
        files["app_gradle"] = files["app_gradle"].replace(
            'System.getenv("TERMUX_EXTERNAL_BOOTSTRAP_URL_AARCH64") ?: ""',
            'System.getenv("TERMUX_EXTERNAL_BOOTSTRAP_URL_AARCH64") ?: "https://invented.invalid/a.zip"',
        )
        self.invalid(files, "must default")

    def test_loader_release_signing_boundary_required(self) -> None:
        files = fixture()
        files["loader_gradle"] = files["loader_gradle"].replace(
            "TERMUX_RELEASE_KEYSTORE_FILE",
            "REMOVED_RELEASE_KEYSTORE_FILE",
        )
        self.invalid(files, "release signing")

    def test_launcher_gate_required(self) -> None:
        files = fixture()
        files["gate"] = files["gate"].replace(
            "BootstrapLoaderClient.requestIfConfigured",
            "bypassLoader",
        )
        self.invalid(files, "launcher gate")

    def test_source_policy_compiles_and_rejects_unsafe_urls(self) -> None:
        javac = shutil.which("javac")
        java = shutil.which("java")
        if not javac or not java:
            self.skipTest("JDK not installed")
        source = ROOT / module.FILES["policy"]
        harness = r'''
package com.termux.rafacodephi.loader;
public final class BootstrapSourcePolicyHarness {
    private interface Checked { void run() throws Exception; }
    private static void reject(Checked checked) throws Exception {
        boolean rejected = false;
        try { checked.run(); } catch (Exception expected) { rejected = true; }
        if (!rejected) throw new AssertionError("unsafe input accepted");
    }
    public static void main(String[] args) throws Exception {
        java.net.URL origin = BootstrapSourcePolicy.requireInitialUrl("https://example.com/a.zip");
        java.net.URL redirected = BootstrapSourcePolicy.requireSameOriginRedirect(
            origin, origin, "/b.zip");
        if (!"example.com".equals(redirected.getHost())) throw new AssertionError();
        BootstrapSourcePolicy.requireAbi("aarch64");
        BootstrapSourcePolicy.requireSha256("a".repeat(64));
        reject(() -> BootstrapSourcePolicy.requireInitialUrl("http://example.com/a.zip"));
        reject(() -> BootstrapSourcePolicy.requireInitialUrl("https://u:p@example.com/a.zip"));
        reject(() -> BootstrapSourcePolicy.requireInitialUrl("https://example.com:444/a.zip"));
        reject(() -> BootstrapSourcePolicy.requireInitialUrl("https://example.com/a.zip#fragment"));
        reject(() -> BootstrapSourcePolicy.requireSameOriginRedirect(
            origin, origin, "https://other.example/b.zip"));
        reject(() -> BootstrapSourcePolicy.requireAbi("mips"));
        reject(() -> BootstrapSourcePolicy.requireSha256("00"));
    }
}
'''
        with tempfile.TemporaryDirectory() as temp_dir:
            temp = Path(temp_dir)
            harness_path = temp / "BootstrapSourcePolicyHarness.java"
            harness_path.write_text(harness, encoding="utf-8")
            subprocess.run(
                [javac, "-source", "11", "-target", "11", "-d", str(temp),
                 str(source), str(harness_path)],
                check=True,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True,
            )
            subprocess.run(
                [java, "-cp", str(temp),
                 "com.termux.rafacodephi.loader.BootstrapSourcePolicyHarness"],
                check=True,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True,
            )


if __name__ == "__main__":
    unittest.main()
