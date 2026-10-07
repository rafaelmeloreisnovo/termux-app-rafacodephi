from pathlib import Path
import importlib.util
import json
import unittest

ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    "token_vazio_validator", ROOT / "tools" / "validate_token_vazio_dictionary.py"
)
assert SPEC and SPEC.loader
MOD = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MOD)

class TokenVazioDictionaryTests(unittest.TestCase):
    def setUp(self):
        self.data = json.loads((ROOT / "configs" / "token-vazio-dictionary.v1.json").read_text(encoding="utf-8"))

    def test_dictionary_is_structurally_valid(self):
        self.assertEqual(MOD.validate_dictionary(self.data), [])

    def test_every_type_is_fail_closed_for_claims(self):
        self.assertTrue(self.data["types"])
        self.assertTrue(all(item["claim_allowed"] is False for item in self.data["types"]))

    def test_core_integrity_families_exist(self):
        codes = {item["code"] for item in self.data["types"]}
        required = {
            "TOKEN_VAZIO_EVIDENCE_INSUFFICIENT",
            "TOKEN_VAZIO_PROVENANCE_UNBOUND",
            "TOKEN_VAZIO_PRIVACY_BLOCKED",
            "TOKEN_VAZIO_SECURITY_BLOCKED",
            "TOKEN_VAZIO_LEGAL_REVIEW",
            "TOKEN_VAZIO_GOVERNANCE_BLOCKED",
            "TOKEN_VAZIO_RISK_UNRESOLVED",
            "TOKEN_VAZIO_CONFORMANCE_UNPROVEN",
        }
        self.assertTrue(required <= codes)

    def test_non_empty_states_are_not_collapsed_into_token_vazio(self):
        states = set(self.data["non_empty_states"])
        self.assertTrue({"PASS","FAIL","NOT_APPLICABLE","NOT_REQUESTED"} <= states)

if __name__ == "__main__":
    unittest.main()
