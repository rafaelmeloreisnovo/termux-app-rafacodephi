"""Guard upload-artifact paths against Debian epoch ':' filename rejection.

Source-only contract. This does not assert APK or physical runtime success.
"""

import unittest
from pathlib import Path


WORKFLOW = (
    Path(__file__).resolve().parents[1]
    / ".github/workflows/freestanding-enterprise-closure.yml"
)


class FreestandingCustodyPathsTest(unittest.TestCase):
    def test_upload_keeps_bounded_evidence_without_unsafe_deb_tree(self):
        source = WORKFLOW.read_text(encoding="utf-8")
        step = "      - name: Upload enterprise candidate and custody evidence\n"
        self.assertEqual(source.count(step), 1)
        upload = source.split(step, 1)[1]
        self.assertIn("if: always()", upload)
        self.assertIn("          path: |\n", upload)
        path_lines = upload.split("          path: |\n", 1)[1].splitlines()
        paths = {line.strip() for line in path_lines if line.startswith("            ")}

        self.assertNotIn("termux-packages/artifacts/rafcodephi-bootstrap/", paths)
        self.assertNotIn("termux-packages/artifacts/rafcodephi-bootstrap/debs/", paths)
        expected = {
            "build/reports/",
            "termux-packages/build/reports/",
            "termux-packages/artifacts/rafcodephi-bootstrap/RAFCODEPHI_REAL_BOOTSTRAP_MANIFEST.txt",
            "termux-packages/artifacts/rafcodephi-bootstrap/rafcodephi-bootstrap-*.zip",
            "termux-packages/artifacts/rafcodephi-bootstrap/rafcodephi-bootstrap-*-symlink-repair.json",
            "termux-packages/artifacts/rafcodephi-bootstrap/debs/*/SHA256SUMS",
        }
        self.assertTrue(expected.issubset(paths), expected - paths)

    def test_heavy_build_still_depends_on_fast_contracts(self):
        source = WORKFLOW.read_text(encoding="utf-8")
        self.assertIn("    needs: contract-fast\n", source)
        self.assertIn("CLAIM_ALLOWED: 'false'", source)
        self.assertIn("PHYSICAL_ANDROID: 'TOKEN_VAZIO'", source)


if __name__ == "__main__":
    unittest.main()
