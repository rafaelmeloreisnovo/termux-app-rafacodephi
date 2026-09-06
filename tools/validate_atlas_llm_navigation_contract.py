#!/usr/bin/env python3
"""Fail-closed structural validator for the Atlas/NOVO LLM navigation contract.

Uses only the Python standard library. This validates the project contract and
critical safety/evidence invariants; it is not a replacement for a full
Draft-07 JSON Schema implementation.
"""

from __future__ import annotations

import argparse
import json
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
CONTRACT = ROOT / "docs/contracts/ATLAS_NOVO_LLM_NAVIGATION_CONTRACT_V1.json"
SCHEMA = ROOT / "docs/contracts/atlas_llm_context_envelope.schema.json"
DEFAULT_FIXTURE = ROOT / "tests/fixtures/atlas_llm_context_envelope.min.v1.json"

EXPECTED_ROUTE = ["ATLAS:X", "NOVO:X", "L:X", "LEARN:X"]
EXPECTED_BACKEND = "LLAMA_LOCAL_RMRCTI"


def load_json(path: Path):
    with path.open("r", encoding="utf-8") as fh:
        return json.load(fh)


def require(condition: bool, message: str, errors: list[str]) -> None:
    if not condition:
        errors.append(message)


def validate_contract(doc: dict, errors: list[str]) -> None:
    require(doc.get("schema") == "rafaelia.atlas_novo_llm_navigation.v1",
            "contract.schema mismatch", errors)
    require(doc.get("claim_allowed") is False,
            "contract must keep claim_allowed=false", errors)
    require(doc.get("append_only") is True,
            "contract must be append_only", errors)
    require(doc.get("route") == EXPECTED_ROUTE,
            "route must be ATLAS:X -> NOVO:X -> L:X -> LEARN:X", errors)

    backend = doc.get("backend", {})
    canonical = backend.get("canonical", {})
    require(canonical.get("id") == EXPECTED_BACKEND,
            "canonical backend must be LLAMA_LOCAL_RMRCTI", errors)
    require(backend.get("external_gpt") == "TOKEN_VAZIO_UNTIL_EXPLICIT_PROVIDER_BINDING",
            "external provider must remain TOKEN_VAZIO until bound", errors)
    require(backend.get("gaia_nanogpt") == "RETRIEVAL_DEMO_ONLY",
            "GAIA NanoGPT must remain retrieval/demo-only", errors)

    data_modes = doc.get("data_modes", {})
    require(data_modes.get("train_or_finetune_weights") == "DISABLED_UNTIL_EXPLICIT_GATES",
            "weight training must remain disabled until explicit gates", errors)

    delta_p = doc.get("rmrcti", {}).get("delta_p", {})
    require(delta_p.get("target") == 0.18,
            "RMRCTI Delta-P target must remain 0.18 in V1 contract", errors)
    require(delta_p.get("classification") == "MEASURED_ASSOCIATION_CANDIDATE",
            "Delta-P must remain a measured-association candidate", errors)
    require("attractor not established" in delta_p.get("required_label", ""),
            "Delta-P label must preserve attractor-not-established boundary", errors)

    voynich = doc.get("voynich", {})
    require(voynich.get("private_body_public_copy") is False,
            "Voynich private body must not be copied to public routing artifacts", errors)

    inv = set(doc.get("invariants", []))
    for required in {
        "RETRIEVAL != TRAINING",
        "MODEL_OUTPUT != EVIDENCE",
        "REPETITION != CAUSALITY",
        "MEASURED_DELTA_P != ATTRACTOR",
        "PRIVATE_POINTER != PUBLIC_DISCLOSURE",
        "TOKEN_VAZIO != 0",
    }:
        require(required in inv, f"missing invariant: {required}", errors)


def validate_schema(doc: dict, errors: list[str]) -> None:
    require(doc.get("$id") == "rafaelia.atlas_llm_context_envelope.v1",
            "context envelope schema id mismatch", errors)
    required = set(doc.get("required", []))
    for field in {"envelope_id", "route", "query", "source_refs", "model_backend", "token_vazio", "claim_allowed"}:
        require(field in required, f"schema missing required field: {field}", errors)

    claim = doc.get("properties", {}).get("claim_allowed", {})
    require(claim.get("const") is False,
            "schema must enforce claim_allowed=false", errors)


def validate_fixture(doc: dict, errors: list[str]) -> None:
    require(doc.get("claim_allowed") is False,
            "fixture claim_allowed must be false", errors)
    route = doc.get("route", {})
    require([route.get("atlas"), route.get("novo"), route.get("longitudinal"), route.get("learn")] == EXPECTED_ROUTE,
            "fixture route mismatch", errors)
    backend = doc.get("model_backend", {})
    require(backend.get("backend_id") == EXPECTED_BACKEND,
            "fixture backend mismatch", errors)
    require(backend.get("training_mode") == "NO_WEIGHT_UPDATE",
            "fixture must not update model weights", errors)
    dp = doc.get("delta_p_evidence")
    if dp is not None:
        require(dp.get("attractor_established") is False,
                "fixture cannot establish Delta-P attractor", errors)


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--fixture", type=Path, default=DEFAULT_FIXTURE)
    args = ap.parse_args()

    errors: list[str] = []
    try:
        contract = load_json(CONTRACT)
        schema = load_json(SCHEMA)
        fixture = load_json(args.fixture)
    except (OSError, json.JSONDecodeError) as exc:
        print(f"FAIL: {exc}", file=sys.stderr)
        return 2

    validate_contract(contract, errors)
    validate_schema(schema, errors)
    validate_fixture(fixture, errors)

    if errors:
        for err in errors:
            print(f"FAIL: {err}", file=sys.stderr)
        return 1

    print("PASS atlas-novo-llm-navigation-contract-v1")
    print(f"route={' -> '.join(EXPECTED_ROUTE)}")
    print(f"backend={EXPECTED_BACKEND}")
    print("claim_allowed=false")
    print("training=NO_WEIGHT_UPDATE")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
