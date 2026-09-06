#!/usr/bin/env python3
from __future__ import annotations

import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
PIN = ROOT / "docs/contracts/ATLAS_LLAMA_PROVIDER_PIN_V1.json"


def main() -> int:
    doc = json.loads(PIN.read_text(encoding="utf-8"))
    assert doc["schema"] == "rafaelia.atlas_llama_provider_pin.v1"
    assert doc["repository"] == "rafaelmeloreisnovo/llamaRafaelia"
    commit = doc["commit"]
    assert isinstance(commit, str) and len(commit) == 40
    assert all(ch in "0123456789abcdef" for ch in commit)
    assert doc["path"] == "rmrCti/atlas_intent_provider_v1.py"
    assert doc["provider_role"] == "LOCAL_INTENT_PROVIDER"
    assert doc["runtime_content_sha256"] == "REQUIRED_AT_INVOCATION"
    assert doc["runtime_content_sha256_enforced_by"] == "tools/atlas_llama_governance_bridge.py"
    assert doc["state"] == "PROVIDER_GIT_PINNED"
    assert doc["model_runtime_proven"] is False
    assert doc["device_reproduction_proven"] is False
    assert doc["claim_allowed"] is False
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
