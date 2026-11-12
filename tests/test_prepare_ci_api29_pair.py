#!/usr/bin/env python3
"""Host-only custody tests for the replacement API29 source pair."""
import hashlib
import importlib.util
import json
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch
import zipfile

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location(
    "api29_source_pair", ROOT / "scripts/prepare_ci_api29_pair.py")
api29 = importlib.util.module_from_spec(spec)
spec.loader.exec_module(api29)


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


class SourcePairTests(unittest.TestCase):
    def test_producer_provenance_is_immutable(self):
        self.assertEqual(api29.PRODUCER_RUN_ID, 43126603251)
        self.assertEqual(api29.PRODUCER_COMMIT,
                         "9b293d4a527dca67350f60a7344ec4b9b37709c2")
        self.assertEqual(set(api29.INPUTS), {"arm", "aarch64"})
        self.assertEqual(len({x[0] for x in api29.INPUTS.values()}), 2)
        self.assertEqual(len({x[1] for x in api29.INPUTS.values()}), 2)
        for transport_sha, payload_sha in api29.INPUTS.values():
            self.assertEqual(len(transport_sha), 64)
            self.assertEqual(len(payload_sha), 64)

    def fixture(self, root, bad_arch=None, bad_kind=None):
        root = Path(root)
        inputs = root / "inputs"
        inputs.mkdir()
        sealer = root / "sealer.sh"
        sealer.write_text(
            'python3 - "$OUT_DIR" "$REPOSITORY_URL" "$TRUST_MODE" '
            '"$PUBLIC_KEY_FILE" "$TARGET_PREFIX" <<\'PY\'\npass\nPY\n')
        key = root / "key.gpg"
        key.write_bytes(b"test-independent-public-key")
        hashes = {}
        for arch in ("arm", "aarch64"):
            payload = ("zip-payload-" + arch).encode()
            source_sha = hashlib.sha256(payload).hexdigest()
            declared_sha = source_sha
            api = "29"
            prefix_state = "PASS_SCOPED"
            if arch == bad_arch:
                if bad_kind == "sha":
                    declared_sha = "0" * 64
                elif bad_kind == "api":
                    api = "30"
                elif bad_kind == "prefix":
                    prefix_state = "BLOCKED"
            manifest = (
                "schema=rafcodephi.real-bootstrap-sourcebuild/v1\n"
                "package_name=com.termux.rafacodephi\n"
                f"sha256_{arch}={declared_sha}\n"
                f"android_api_target_{arch}={api}\n"
                f"prefix_gate_state_{arch}={prefix_state}\n"
            )
            artifact = inputs / (arch + ".zip")
            with zipfile.ZipFile(artifact, "w") as z:
                z.writestr(
                    f"artifacts/rafcodephi-bootstrap/rafcodephi-bootstrap-{arch}.zip",
                    payload)
                z.writestr(
                    "artifacts/rafcodephi-bootstrap/RAFCODEPHI_REAL_BOOTSTRAP_MANIFEST.txt",
                    manifest)
            hashes[arch] = (sha(artifact), source_sha)
        return inputs, sealer, key, hashes

    def run_fixture(self, bad_arch=None, bad_kind=None):
        with tempfile.TemporaryDirectory() as root:
            inputs, sealer, key, hashes = self.fixture(
                root, bad_arch=bad_arch, bad_kind=bad_kind)
            with patch.object(api29, "INPUTS", hashes), \
                 patch.object(api29, "KEY_SHA256", sha(key)), \
                 patch.object(api29, "SEALER_SHA256", sha(sealer)):
                return api29.prepare(inputs, Path(root) / "out", sealer, key)

    def test_sha_bound_pair_is_accepted_in_custody_scope(self):
        self.run_fixture()

    def test_manifest_payload_hash_mismatch_blocked(self):
        with self.assertRaisesRegex(ValueError, "producer manifest bootstrap digest"):
            self.run_fixture(bad_arch="arm", bad_kind="sha")

    def test_manifest_api30_legacy_rejected(self):
        with self.assertRaisesRegex(ValueError, "did not attest API29"):
            self.run_fixture(bad_arch="arm", bad_kind="api")

    def test_source_prefix_gate_failure_rejected(self):
        with self.assertRaisesRegex(ValueError, "producer prefix gate"):
            self.run_fixture(bad_arch="aarch64", bad_kind="prefix")


if __name__ == "__main__":
    unittest.main()
