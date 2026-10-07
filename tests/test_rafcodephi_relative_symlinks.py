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


if __name__ == "__main__":
    unittest.main()
