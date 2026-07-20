from __future__ import annotations

import importlib.util
import json
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
VALIDATOR = ROOT / "tools" / "validate_runtime_evidence.py"
COLLECTOR = ROOT / "tools" / "collect_runtime_receipt_v2.py"

spec = importlib.util.spec_from_file_location("runtime_evidence", VALIDATOR)
assert spec and spec.loader
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)


def collect(tmp_path: Path) -> dict:
    output = tmp_path / "evidence.json"
    subprocess.run([sys.executable, str(COLLECTOR), str(output)], check=True)
    return json.loads(output.read_text(encoding="utf-8"))


def test_collector_host_receipt_is_valid(tmp_path: Path) -> None:
    data = collect(tmp_path)
    assert data["adapter_state"] == "IMPLEMENTED"
    assert data["evidence_state"] in {
        "HOST_SIMULATION",
        "DEVICE_OBSERVED_INCOMPLETE",
    }
    assert data["execution_evidence_usable"] is False
    assert module.validate(data) == []


def test_claim_promotion_rejected(tmp_path: Path) -> None:
    data = collect(tmp_path)
    data["claim_allowed"] = True
    data["receipt_sha256"] = module.canonical_digest(data)
    assert module.validate(data)


def test_mutation_rejected(tmp_path: Path) -> None:
    data = collect(tmp_path)
    data["install_or_mutation_performed"] = True
    data["receipt_sha256"] = module.canonical_digest(data)
    assert module.validate(data)


def test_tampered_receipt_digest_rejected(tmp_path: Path) -> None:
    data = collect(tmp_path)
    data["package_name"] = "tampered.package"
    assert "receipt_sha256 mismatch" in module.validate(data)


def test_incomplete_device_cannot_claim_complete(tmp_path: Path) -> None:
    data = collect(tmp_path)
    data["device"]["model"] = "model"
    data["device"]["abi_primary"] = "arm64-v8a"
    data["evidence_state"] = "DEVICE_RECEIPT_COMPLETE"
    data["execution_evidence_usable"] = True
    data["receipt_sha256"] = module.canonical_digest(data)
    errors = module.validate(data)
    assert any("derive" in error or "completeness" in error for error in errors)


def test_command_probe_set_is_stable(tmp_path: Path) -> None:
    data = collect(tmp_path)
    assert {item["name"] for item in data["commands"]} == {
        "sh",
        "ls",
        "pkg",
        "apt",
        "dpkg",
        "proot",
    }


def test_cli_can_require_real_device_receipt(tmp_path: Path) -> None:
    output = tmp_path / "evidence.json"
    subprocess.run([sys.executable, str(COLLECTOR), str(output)], check=True)
    result = subprocess.run(
        [
            sys.executable,
            str(VALIDATOR),
            str(output),
            "--require-device-complete",
        ],
        check=False,
    )
    assert result.returncode == 1
