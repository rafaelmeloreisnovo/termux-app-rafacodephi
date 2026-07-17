from __future__ import annotations

import importlib.util
import json
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
VALIDATOR = ROOT / "tools" / "validate_runtime_evidence.py"
spec = importlib.util.spec_from_file_location("runtime_evidence", VALIDATOR)
assert spec and spec.loader
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)


def fixture() -> dict:
    return json.loads((ROOT / "examples" / "runtime_evidence.token-vazio.json").read_text())


def test_fixture_valid() -> None:
    assert module.validate(fixture()) == []


def test_claim_promotion_rejected() -> None:
    data = fixture()
    data["claim_allowed"] = True
    assert module.validate(data)


def test_mutation_rejected() -> None:
    data = fixture()
    data["install_or_mutation_performed"] = True
    assert module.validate(data)


def test_collector_runs_without_mutation(tmp_path: Path) -> None:
    output = tmp_path / "evidence.json"
    subprocess.run(
        ["sh", str(ROOT / "scripts" / "federation" / "collect_runtime_evidence.sh"), str(output)],
        check=True,
    )
    data = json.loads(output.read_text())
    assert data["install_or_mutation_performed"] is False
    assert module.validate(data) == []
