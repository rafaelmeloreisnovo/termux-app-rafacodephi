"""Regression for canonical ./ symlink records without external dependencies."""
import importlib.util
import io
from pathlib import Path
import unittest
import zipfile

ROOT = Path(__file__).resolve().parents[1]


class SymlinkTests(unittest.TestCase):
    def test_importer_prefixed_and_duplicate(self):
        path = ROOT / "scripts/import_rafcodephi_real_bootstrap.py"
        spec = importlib.util.spec_from_file_location("raf_importer", path)
        importer = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(importer)
        def parse(text):
            buf = io.BytesIO()
            with zipfile.ZipFile(buf, "w") as zf:
                zf.writestr("SYMLINKS.txt", text)
            buf.seek(0)
            with zipfile.ZipFile(buf) as zf:
                return importer.parse_symlink_destinations(zf, set(zf.namelist()))
        self.assertEqual(parse("dash←./bin/sh\ntermux-api-broadcast←./libexec/termux-api\n"),
                         {"bin/sh", "libexec/termux-api"})
        with self.assertRaises(SystemExit):
            parse("dash←./bin/sh\ndash←bin/sh\n")


    def test_rights_gated_artifact_custody(self):
        workflow = (ROOT / ".github/workflows/freestanding-enterprise-closure.yml").read_text()
        self.assertIn("BLOCKED_UNVERIFIED_LICENSE_RIGHTS", workflow)
        self.assertIn("sha256sum -c SHA256SUMS", workflow)
        upload = workflow.split("- name: Upload enterprise candidate and custody evidence", 1)[1]
        self.assertNotIn("            termux-packages/artifacts/rafcodephi-bootstrap/\n", upload)
        self.assertIn("RAFCODEPHI_REAL_BOOTSTRAP_MANIFEST.txt", upload)

    def test_candidate_remains_unpromoted(self):
        import json
        pinned = json.loads(
            (ROOT / "data/contracts/termux-packages-rafcodephi-pin.v1.json").read_text()
        )
        candidate = pinned["channels"]["candidate"]
        self.assertFalse(candidate["claim_allowed"])
        self.assertEqual("TOKEN_VAZIO", candidate["physical_android"])
        self.assertNotEqual("MERGED_BASELINE", candidate["state"])


if __name__ == "__main__":
    unittest.main()
