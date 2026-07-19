#!/usr/bin/env python3

from __future__ import annotations

import importlib.util
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
MODULE_PATH = ROOT / "scripts" / "raf_compile_warning_contract.py"
SPEC = importlib.util.spec_from_file_location("raf_compile_warning_contract", MODULE_PATH)
assert SPEC and SPEC.loader
MODULE = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = MODULE
SPEC.loader.exec_module(MODULE)


class WarningContractTest(unittest.TestCase):
    def test_unused_function_is_gc_candidate_not_auto_delete(self) -> None:
        diagnostics = MODULE.parse_lines(
            ["core.c:12:3: warning: unused function 'x' [-Wunused-function]\n"]
        )
        self.assertEqual(len(diagnostics), 1)
        item = diagnostics[0]
        self.assertEqual(item.category, "GC_CANDIDATE")
        self.assertFalse(item.auto_delete)
        self.assertEqual(item.severity, "review")

    def test_unused_parameter_maps_to_intentional_void_review(self) -> None:
        diagnostics = MODULE.parse_lines(
            ["jni.c:7:44: warning: unused parameter 'env' [-Wunused-parameter]\n"]
        )
        self.assertEqual(diagnostics[0].category, "INTENTIONAL_VOID_OR_API_FIX")
        self.assertIn("RAF_DISCARD", diagnostics[0].action)

    def test_ignored_result_blocks_release(self) -> None:
        diagnostics = MODULE.parse_lines(
            ["io.c:9:2: warning: ignoring return value [-Wunused-result]\n"]
        )
        report = MODULE.build_report(diagnostics)
        self.assertFalse(report["release_allowed"])
        self.assertEqual(report["severities"]["blocker"], 1)

    def test_compiler_error_blocks_release(self) -> None:
        diagnostics = MODULE.parse_lines(
            ["core.c:1:1: error: unknown type name 'u128'\n"]
        )
        self.assertEqual(diagnostics[0].category, "COMPILER_ERROR")
        self.assertFalse(MODULE.build_report(diagnostics)["release_allowed"])

    def test_non_diagnostic_lines_are_ignored(self) -> None:
        diagnostics = MODULE.parse_lines(["clang version 20\n", "build complete\n"])
        self.assertEqual(diagnostics, [])


if __name__ == "__main__":
    unittest.main()
