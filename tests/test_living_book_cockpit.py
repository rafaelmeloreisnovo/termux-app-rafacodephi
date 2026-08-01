#!/usr/bin/env python3
from __future__ import annotations

import importlib.util
import json
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location("cockpit", ROOT / "scripts" / "living_book_cockpit.py")
cockpit = importlib.util.module_from_spec(SPEC)
assert SPEC.loader is not None
SPEC.loader.exec_module(cockpit)


def contract():
    return json.loads((ROOT / "configs" / "living-book-cockpit-v1.json").read_text(encoding="utf-8"))


def bundle():
    body = {
        "source": {
            "cell_id": "LBC-MUSIC-0001",
            "cell_digests": {"sha256": "a" * 64},
            "ir_digests": {"sha256": "b" * 64}
        },
        "payload": {
            "intent_id": "INT-001",
            "module_id": "support.math",
            "action": "PROPOSE_ANALYSIS",
            "ir_embedded": False,
            "private_source_embedded": False
        },
        "policy": {
            "transport_mode": "DESCRIPTOR_ONLY",
            "human_approval_state": "REQUIRED_BEFORE_DISPATCH",
            "human_approval_digest": None,
            "dispatch_allowed": False,
            "execution_allowed": False,
            "publication_allowed": False,
            "claim_allowed": False,
            "network_target": None
        }
    }
    return {
        "schema": "rafgittools.living-book-ir-bundle/v1",
        "bundle_id": "LBB-MUSIC-0001",
        "state": "READY_FOR_REVIEW_NOT_DISPATCHED",
        **body,
        "integrity": {"digests": cockpit.digests(body)}
    }


class CockpitTests(unittest.TestCase):
    def test_contract(self):
        self.assertEqual([], cockpit.validate_contract(contract()))

    def test_valid_bundle(self):
        self.assertEqual([], cockpit.validate_bundle(bundle(), contract()))

    def test_dispatch_rejected(self):
        value = bundle()
        value["policy"]["dispatch_allowed"] = True
        value["integrity"]["digests"] = cockpit.digests({key: value[key] for key in ("source", "payload", "policy")})
        self.assertTrue(any("dispatch_allowed" in error for error in cockpit.validate_bundle(value, contract())))

    def test_execution_rejected(self):
        value = bundle()
        value["policy"]["execution_allowed"] = True
        value["integrity"]["digests"] = cockpit.digests({key: value[key] for key in ("source", "payload", "policy")})
        self.assertTrue(any("execution_allowed" in error for error in cockpit.validate_bundle(value, contract())))

    def test_private_key_rejected(self):
        value = bundle()
        value["payload"]["seed"] = "private"
        value["integrity"]["digests"] = cockpit.digests({key: value[key] for key in ("source", "payload", "policy")})
        self.assertTrue(any("sensitive key" in error for error in cockpit.validate_bundle(value, contract())))

    def test_tamper_rejected(self):
        value = bundle()
        value["payload"]["module_id"] = "tampered"
        self.assertTrue(any("digest mismatch" in error for error in cockpit.validate_bundle(value, contract())))

    def test_receipt_never_claims_execution(self):
        result = cockpit.receipt(bundle(), [], "armv7l")
        self.assertFalse(result["execution_performed"])
        self.assertFalse(result["claim_allowed"])

    def test_menu_has_governance_surfaces(self):
        labels = {item["label"] for item in contract()["menu"]}
        self.assertTrue({"Espelhos humano / IA", "Permissões", "Privacidade e segurança", "Auditoria"}.issubset(labels))

    def test_render_is_inspect_only(self):
        text = cockpit.render(contract(), bundle(), [])
        self.assertIn("execution: BLOCKED", text)
        self.assertIn("PASS_INSPECT_ONLY", text)


if __name__ == "__main__":
    unittest.main()
