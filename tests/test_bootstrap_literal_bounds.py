# SPDX-License-Identifier: GPL-3.0-only
# RAFCODEPHI local contribution (2026-11-01): bounded literal-log regression test.
# The modified Termux-derived application keeps upstream notices and licenses.
from pathlib import Path
import json
import re
import unittest

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / "src/bootstrap/bootstrap_orchestrator.c"
TOKENS = ROOT / "configs/token-vazio-dictionary.v1.json"
PAIR = re.compile(r'"((?:\\.|[^"\\])*)"\s*,\s*RAF_LIT_LEN\("((?:\\.|[^"\\])*)"\)\)')
MANUAL = re.compile(r'"(?:\\.|[^"\\])*"\s*,\s*\d+u?\s*\)')


def literal_contract(source: str) -> bool:
    pairs = PAIR.findall(source)
    return (len(pairs) == 9 and all(left == right for left, right in pairs)
            and not MANUAL.search(source))


class BootstrapLiteralBoundsTest(unittest.TestCase):
    def test_all_bootstrap_literal_logs_use_compiler_lengths(self):
        source = SRC.read_text(encoding="utf-8")
        self.assertIn("#define RAF_LIT_LEN(", source)
        self.assertTrue(literal_contract(source))

    def test_falsifier_rejects_wrong_literal_length_reference(self):
        source = SRC.read_text(encoding="utf-8")
        mutated = source.replace(
            'RAF_LIT_LEN("RAF-BOOTSTRAP: RAF-NINJA proot\\n")',
            'RAF_LIT_LEN("X")',
            1
        )
        self.assertNotEqual(mutated, source)
        self.assertFalse(literal_contract(mutated))

    def test_utility_gap_is_not_provenance_or_zero(self):
        dictionary = json.loads(TOKENS.read_text(encoding="utf-8"))
        items = {x["code"]: x for x in dictionary["types"]}
        self.assertIn("TOKEN_VAZIO_UTIL", items)
        self.assertIn("TOKEN_VAZIO_PROVENANCE_UNBOUND", items)
        self.assertNotEqual(items["TOKEN_VAZIO_UTIL"]["family"],
                            items["TOKEN_VAZIO_PROVENANCE_UNBOUND"]["family"])
        self.assertFalse(items["TOKEN_VAZIO_UTIL"]["claim_allowed"])
        self.assertFalse(items["TOKEN_VAZIO_PROVENANCE_UNBOUND"]["claim_allowed"])


if __name__ == "__main__":
    unittest.main()
