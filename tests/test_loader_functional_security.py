from __future__ import annotations

import copy
import importlib.util
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    "loader_security", ROOT / "tools" / "validate_loader_functional_security.py")
assert SPEC and SPEC.loader
module = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(module)


def current_fixture():
    return module.load(ROOT)


def stub_fixture():
    files = current_fixture()
    for key in ("install_contract", "activity", "service", "source_policy", "provider"):
        files[key] = ""
    files["manifest"] = '''<manifest><application
android:hasCode="false" android:debuggable="false">
<meta-data android:name="CONTRACT_STATE" android:value="STUB_NO_BOOTSTRAP_PAYLOAD"/>
</application></manifest>'''
    files["readme"] = "\n".join((
        "has_code = false",
        "installer_behavior = absent",
        "release_allowed = false",
        "BLOCKED_BY[LOADER_FUNCTIONAL_CONTRACT_REQUIRED]",
    ))
    files["loader_gradle"] = 'versionName "0.1.0-stub"\n// Builds the loader stub\n'
    files["artifact_verifier"] = "\n".join((
        "state=STUB_NO_BOOTSTRAP_PAYLOAD",
        "manifest_has_code_false",
        "dex_policy=no_dex",
    ))
    return files


def functional_fixture():
    files = stub_fixture()
    files["manifest"] = '''<manifest>
<uses-permission android:name="android.permission.INTERNET"/>
<uses-permission android:name="com.termux.rafacodephi.permission.BOOTSTRAP_HANDOFF"/>
<application android:usesCleartextTraffic="false">
<meta-data android:name="CONTRACT_STATE" android:value="BOOTSTRAP_ACQUIRE_HANDOFF_CAPABLE"/>
<activity android:exported="true" android:permission="com.termux.rafacodephi.permission.BOOTSTRAP_HANDOFF"/>
<service android:exported="false"/>
<provider android:exported="false" android:grantUriPermissions="true"/>
</application></manifest>'''
    files["install_contract"] = "HANDOFF_PERMISSION PROVIDER_AUTHORITY ACTION_BOOTSTRAP_VERIFIED"
    files["activity"] = "requireAbi requireSha256 requireInitialUrl"
    files["service"] = "MAX_DOWNLOAD_BYTES setInstanceFollowRedirects(false) SHA256_MISMATCH getFD().sync() grantUriPermission setPackage"
    files["source_policy"] = '"https" MAX_REDIRECTS sameOrigin NON_STANDARD_HTTPS_PORT_BLOCKED CROSS_ORIGIN_REDIRECT_BLOCKED'
    files["provider"] = "MODE_READ_ONLY READ_ONLY_PROVIDER getCanonicalPath"
    files["host_client"] = "checkSignatures SIGNATURE_MATCH EXTERNAL_CANONICAL_BLAKE3_NOT_CONFIGURED"
    files["host_receiver"] = 'MessageDigest.getInstance("SHA-256") blake3Hex MAX_ZIP_ENTRIES MAX_UNCOMPRESSED_BYTES revokeUriPermission Os.rename'
    files["host_gate"] = "BootstrapGateActivity"
    files["host_integrity"] = "blake3Hex"
    files["host_manifest"] = 'android:protectionLevel="signature" BootstrapHandoffReceiver BootstrapGateActivity'
    files["native"] = "O_NOFOLLOW errno == ENOENT S_ISREG"
    envs = (
        "TERMUX_EXTERNAL_BOOTSTRAP_URL_AARCH64", "TERMUX_EXTERNAL_BOOTSTRAP_URL_ARM",
        "TERMUX_EXTERNAL_BOOTSTRAP_URL_I686", "TERMUX_EXTERNAL_BOOTSTRAP_URL_X86_64",
        "TERMUX_EXTERNAL_BOOTSTRAP_SHA256_AARCH64", "TERMUX_EXTERNAL_BOOTSTRAP_SHA256_ARM",
        "TERMUX_EXTERNAL_BOOTSTRAP_SHA256_I686", "TERMUX_EXTERNAL_BOOTSTRAP_SHA256_X86_64",
    )
    files["pin_gradle"] = "\n".join(f'System.getenv("{env}") ?: ""' for env in envs)
    files["loader_gradle"] += "\n" + " ".join((
        "TERMUX_ENABLE_RELEASE_SIGNING", "TERMUX_RELEASE_KEYSTORE_FILE",
        "TERMUX_RELEASE_KEYSTORE_PASSWORD", "TERMUX_RELEASE_KEY_ALIAS",
        "TERMUX_RELEASE_KEY_PASSWORD",
    ))
    return files


class LoaderFunctionalSecurityTests(unittest.TestCase):

    def assert_invalid(self, files, phrase):
        _, errors = module.validate_snapshot(files)
        self.assertTrue(errors)
        self.assertIn(phrase, "\n".join(errors))

    def test_current_loader_is_functional_and_security_gated(self):
        state, errors = module.validate_snapshot(current_fixture())
        self.assertEqual("FUNCTIONAL_SECURITY_GATED", state)
        self.assertEqual([], errors)

    def test_complete_functional_fixture_is_accepted_as_gated(self):
        state, errors = module.validate_snapshot(functional_fixture())
        self.assertEqual("FUNCTIONAL_SECURITY_GATED", state)
        self.assertEqual([], errors)

    def test_stub_without_has_code_false_is_rejected(self):
        files = stub_fixture()
        files["manifest"] = files["manifest"].replace('android:hasCode="false"', '')
        self.assert_invalid(files, "hasCode=false")

    def test_stub_cannot_claim_functionality(self):
        files = stub_fixture()
        files["readme"] += "\nfunctional_installer = true\n"
        self.assert_invalid(files, "claims functional")

    def test_hybrid_stub_with_java_sources_is_rejected(self):
        files = stub_fixture()
        files["service"] = "MAX_DOWNLOAD_BYTES"
        self.assert_invalid(files, "hybrid state")

    def test_target_directory_contract_is_rejected(self):
        files = functional_fixture()
        files["install_contract"] += " EXTRA_TARGET_DIR target_dir"
        self.assert_invalid(files, "forbidden source token")

    def test_loader_extraction_is_rejected(self):
        files = functional_fixture()
        files["service"] += " ZipInputStream ZipEntry"
        self.assert_invalid(files, "forbidden source token")

    def test_cleartext_or_incomplete_manifest_is_rejected(self):
        files = functional_fixture()
        files["manifest"] = files["manifest"].replace('android:usesCleartextTraffic="false"', 'android:usesCleartextTraffic="true"')
        self.assert_invalid(files, "manifest boundary")

    def test_plaintext_http_literal_is_rejected(self):
        files = functional_fixture()
        files["source_policy"] += " http://example.invalid"
        self.assert_invalid(files, "plaintext HTTP")

    def test_implicit_redirects_are_rejected(self):
        files = functional_fixture()
        files["service"] = files["service"].replace("setInstanceFollowRedirects(false)", "setInstanceFollowRedirects(true)")
        self.assert_invalid(files, "bounded acquisition")

    def test_download_budget_is_required(self):
        files = functional_fixture()
        files["service"] = files["service"].replace("MAX_DOWNLOAD_BYTES", "NO_LIMIT")
        self.assert_invalid(files, "bounded acquisition")

    def test_provider_must_be_read_only(self):
        files = functional_fixture()
        files["provider"] = files["provider"].replace("MODE_READ_ONLY", "MODE_READ_WRITE")
        self.assert_invalid(files, "read-only")

    def test_host_signer_check_is_required(self):
        files = functional_fixture()
        files["host_client"] = files["host_client"].replace("checkSignatures", "skipSignatures")
        self.assert_invalid(files, "signer")

    def test_host_blake3_gate_is_required(self):
        files = functional_fixture()
        files["host_receiver"] = files["host_receiver"].replace("blake3Hex", "removed")
        self.assert_invalid(files, "double-hash")

    def test_external_pins_must_default_empty(self):
        files = functional_fixture()
        files["pin_gradle"] = files["pin_gradle"].replace(
            'System.getenv("TERMUX_EXTERNAL_BOOTSTRAP_URL_AARCH64") ?: ""',
            'System.getenv("TERMUX_EXTERNAL_BOOTSTRAP_URL_AARCH64") ?: "https://invented.invalid"')
        self.assert_invalid(files, "default empty")

    def test_native_heap_allocation_is_rejected(self):
        files = functional_fixture()
        files["native"] += " malloc(1);"
        self.assert_invalid(files, "allocates heap")

    def test_missing_host_boundary_file_is_rejected(self):
        files = functional_fixture()
        files["host_receiver"] = ""
        self.assert_invalid(files, "boundary file missing")

    def test_canonical_workflow_must_execute_gate(self):
        files = stub_fixture()
        files["workflow"] = files["workflow"].replace("validate_loader_functional_security.py", "removed_validator.py")
        self.assert_invalid(files, "does not execute quarantine gate")

    def test_contract_cannot_enable_release(self):
        files = stub_fixture()
        files["contract"] = files["contract"].replace('"release_allowed": false', '"release_allowed": true')
        self.assert_invalid(files, "claim and release disabled")


if __name__ == "__main__":
    unittest.main()
