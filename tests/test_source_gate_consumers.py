#!/usr/bin/env python3
"""Host-only contract: all selected consumers pin the exact clean-source gate."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]
ACTION = "rafaelmeloreisnovo/actions/rafcode/verify-source@7ae0fca1090fa3b94f7f86c4c3951bb52189e213"
LEGACY = "rafaelmeloreisnovo/actions/rafcode/verify-source@7c03495ebe1d28a11ea419799c4f61427250244f"
FILES = (
    ".github/workflows/rafcodephi-signed-apt-import-contract.yml",
    ".github/workflows/beta-real-bootstrap-contract.yml",
    ".github/workflows/termux-health-server.yml",
)
EXPECTED_SHA = "${{ github.event.pull_request.head.sha || github.sha }}"

class ConsumerSourceGateContract(unittest.TestCase):
    def test_all_consumers_bind_the_new_immutable_commit(self):
        for name in FILES:
            with self.subTest(workflow=name):
                text = (ROOT / name).read_text(encoding="utf-8")
                self.assertEqual(text.count(ACTION), 1)
                self.assertNotIn(LEGACY, text)
                self.assertIn("expected-sha: " + EXPECTED_SHA, text)

    def test_checkout_precedes_gate_and_host_code(self):
        for name in FILES:
            with self.subTest(workflow=name):
                text = (ROOT / name).read_text(encoding="utf-8")
                checkout = text.index("uses: actions/checkout@")
                guard = text.index("uses: " + ACTION)
                host_test = text.index("run: python3 -m unittest tests.test_source_gate_consumers -v")
                self.assertLess(checkout, guard)
                self.assertLess(guard, host_test)
                self.assertIn("'tests/test_source_gate_consumers.py'", text)

if __name__ == "__main__":
    unittest.main(verbosity=2)
