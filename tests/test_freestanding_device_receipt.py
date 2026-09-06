#!/usr/bin/env python3
from __future__ import annotations

import importlib.util
import json
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
MODULE_PATH = ROOT / "scripts" / "validate_freestanding_device_receipt.py"
spec = importlib.util.spec_from_file_location("receipt_validator", MODULE_PATH)
validator = importlib.util.module_from_spec(spec)
assert spec and spec.loader
spec.loader.exec_module(validator)


def check(required: bool, state: str = "PASS", exit_code: int = 0) -> dict:
    return {"required": required, "exit_code": exit_code, "state": state}


def receipt(receipt_id: str, created: int, *, claim: bool = True, apk: str | None = None) -> dict:
    sha = "a" * 64
    checks = {
        "gate_probe": check(True),
        "pkg": check(True),
        "proot": check(True),
        "proot_distro": check(True),
        "ninja": check(True),
        "clang": check(True),
        "cmake": check(True),
        "qemu_system_x86_64": check(True),
        "qemu_img": check(True),
    }
    return {
        "schema": validator.SCHEMA,
        "receipt_id": receipt_id,
        "created_unix": created,
        "architecture": "aarch64",
        "prefix": "/data/data/com.termux.rafacodephi/files/usr",
        "gate_sha256": sha,
        "candidate_apk_sha256": apk,
        "phase": "full",
        "runtime_state": "RUNTIME_PROVEN" if claim else "TOKEN_VAZIO",
        "device_state": "DEVICE_PROVEN" if claim else "TOKEN_VAZIO",
        "reproduced_state": "TOKEN_VAZIO",
        "claim_allowed": claim,
        "checks": checks,
    }


class ReceiptValidationTest(unittest.TestCase):
    def write(self, root: Path, name: str, payload: dict) -> Path:
        path = root / name
        path.write_text(json.dumps(payload, sort_keys=True) + "\n", encoding="utf-8")
        return path

    def test_valid_device_proven_receipt(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            path = self.write(Path(tmp), "one.json", receipt("receipt-one", 100, apk="b" * 64))
            result = validator.validate_receipt(path, verify_sidecar=False)
            self.assertTrue(result["valid"], result["errors"])

    def test_rejects_promoted_claim_with_failed_required_probe(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            payload = receipt("receipt-bad", 101, apk="b" * 64)
            payload["checks"]["ninja"] = check(True, "TOKEN_VAZIO", 127)
            path = self.write(Path(tmp), "bad.json", payload)
            result = validator.validate_receipt(path, verify_sidecar=False)
            self.assertFalse(result["valid"])
            self.assertTrue(any("promotion invariant" in item for item in result["errors"]))

    def test_unpromoted_token_vazio_is_valid_when_required_probe_failed(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            payload = receipt("receipt-gap", 102, claim=False, apk="b" * 64)
            payload["checks"]["ninja"] = check(True, "TOKEN_VAZIO", 127)
            path = self.write(Path(tmp), "gap.json", payload)
            result = validator.validate_receipt(path, verify_sidecar=False)
            self.assertTrue(result["valid"], result["errors"])

    def test_full_phase_without_apk_binding_stays_token_vazio_and_valid(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            payload = receipt("receipt-unbound", 103, claim=False, apk=None)
            path = self.write(Path(tmp), "unbound.json", payload)
            result = validator.validate_receipt(path, verify_sidecar=False)
            self.assertTrue(result["valid"], result["errors"])

    def test_full_phase_cannot_claim_without_apk_binding(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            payload = receipt("receipt-unbound-claim", 104, claim=True, apk=None)
            path = self.write(Path(tmp), "unbound-claim.json", payload)
            result = validator.validate_receipt(path, verify_sidecar=False)
            self.assertFalse(result["valid"])
            self.assertTrue(any("promotion invariant" in item for item in result["errors"]))

    def test_two_distinct_bound_receipts_promote_reproduced(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            a = validator.validate_receipt(self.write(root, "a.json", receipt("receipt-run-a", 200, apk="b" * 64)), verify_sidecar=False)
            b = validator.validate_receipt(self.write(root, "b.json", receipt("receipt-run-b", 201, apk="b" * 64)), verify_sidecar=False)
            result = validator.reproduce(a, b)
            self.assertEqual("REPRODUCED", result["reproduced_state"])
            self.assertTrue(result["claim_allowed"])

    def test_reproduction_rejects_same_receipt_or_unbound_apk(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            a = validator.validate_receipt(self.write(root, "a.json", receipt("receipt-same", 300)), verify_sidecar=False)
            b = validator.validate_receipt(self.write(root, "b.json", receipt("receipt-same", 300)), verify_sidecar=False)
            result = validator.reproduce(a, b)
            self.assertEqual("TOKEN_VAZIO", result["reproduced_state"])
            self.assertFalse(result["claim_allowed"])


if __name__ == "__main__":
    unittest.main()
