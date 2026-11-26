#!/usr/bin/env python3
"""A forbidden Java dependency must fail before compilation, even without rg."""
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
JAVA = "app/src/main/java/com/termux/rafacodephi/lowlevel/"


class DependencyGateTests(unittest.TestCase):
    def test_forbidden_dependencies_are_rejected(self):
        injections = ["public static native int foreign();",
                      "public static Object foreign() { return new Object(); }",
                      "public static int foreign() { return Integer.bitCount(1); }"]
        for injection in injections:
            with self.subTest(injection=injection), tempfile.TemporaryDirectory() as tmp:
                target = Path(tmp)
                for rel in ("configs/lowlevel-selection.v2.json", JAVA+"AuthorialBitOps.java", JAVA+"AuthorialBitOpsAdversarial.java"):
                    dst = target/rel
                    dst.parent.mkdir(parents=True, exist_ok=True)
                    shutil.copyfile(ROOT/rel, dst)
                product = target/JAVA/"AuthorialBitOps.java"
                text = product.read_text()
                pos = text.rfind("}")
                product.write_text(text[:pos] + injection + text[pos:])
                result = subprocess.run(["bash", str(ROOT/"scripts/verify_pure_java_bit_core.sh"), str(target)],
                                        capture_output=True, text=True, timeout=15)
                self.assertEqual(result.returncode, 21, result.stdout+result.stderr)
                self.assertIn("UNEXPECTED_LIBRARY_OR_NATIVE_REFERENCE", result.stderr)


if __name__ == "__main__":
    unittest.main()
