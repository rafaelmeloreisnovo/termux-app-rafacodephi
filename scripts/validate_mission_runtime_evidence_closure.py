#!/usr/bin/env python3
"""Fail-closed validator for the RAFAELIA MissionExecution runtime closure.

This validator proves only that the source contract is internally coherent or that a
supplied receipt satisfies the declared evidence shape. It cannot manufacture device,
provider, identity, legal, manual, or credentialed-analysis evidence.
"""

from __future__ import annotations

import argparse
import copy
import json
import re
import sys
from datetime import datetime
from pathlib import Path
from typing import Any

ROOT = Path(__file__).resolve().parents[1]
DEFAULT_CONTRACT = ROOT / "docs/contracts/mission_runtime_evidence_closure.v1.json"
EXPECTED_SCHEMA = "rafaelia.mission_runtime_evidence_closure/v1"
EXPECTED_RECEIPT_SCHEMA = "rafaelia.mission_runtime_closure_receipt/v1"
SHA1_RE = re.compile(r"^[0-9a-f]{40}$")
SHA256_RE = re.compile(r"^[0-9a-f]{64}$")

REQUIRED_GATE_IDS = {
    "physical_android_termux_execution",
    "exact_multi_repository_runtime_execution",
    "remote_network_identity",
    "provider_or_legal_authorization",
    "live_default_branch_ruleset",
    "server_merge_enforcement",
    "manual_promotion_decision",
    "codescan_credentialed_analysis",
}

REQUIRED_INVARIANTS = {
    "DATASET_INFORMS!=MISSION_AUTHORITY",
    "MODEL_PROPOSAL!=EXECUTION_PERMISSION",
    "RETRIEVAL_CONTEXT!=WEIGHT_UPDATE",
    "LEARN_APPEND_ONLY!=ONLINE_SELF_TRAINING",
    "CONTINUE_APPROVED_SCOPE!=AUTONOMOUS_GOAL_CREATION",
    "SOURCE!=EXECUTION!=EVIDENCE!=CLAIM",
    "TOKEN_VAZIO!=0",
    "STATIC_OR_CI_PASS!=PHYSICAL_DEVICE_PROOF",
    "SIMULATION!=EXTERNAL_AUTHORITY",
    "NETWORK_REACHABILITY!=REMOTE_IDENTITY",
    "SECRET_PRESENCE!=ANALYSIS_RESULT",
    "GREEN_GATE_PROMOTES_ONLY_MEASURED_SCOPE",
}


class ValidationError(ValueError):
    pass


def require(condition: bool, message: str) -> None:
    if not condition:
        raise ValidationError(message)


def load_json(path: Path) -> dict[str, Any]:
    try:
        value = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        raise ValidationError(f"cannot read JSON {path}: {exc}") from exc
    require(isinstance(value, dict), f"top-level JSON must be an object: {path}")
    return value


def nonempty(value: Any) -> bool:
    if value is None:
        return False
    if isinstance(value, str):
        return bool(value.strip()) and not value.startswith("TOKEN_VAZIO")
    if isinstance(value, (dict, list, tuple, set)):
        return bool(value)
    return True


def parse_time(value: Any, field: str) -> None:
    require(isinstance(value, str) and value.strip(), f"{field} must be a non-empty timestamp")
    text = value.strip().replace("Z", "+00:00")
    try:
        datetime.fromisoformat(text)
    except ValueError as exc:
        raise ValidationError(f"{field} is not ISO-8601: {value}") from exc


def validate_contract(doc: dict[str, Any]) -> dict[str, dict[str, Any]]:
    require(doc.get("schema") == EXPECTED_SCHEMA, "contract schema mismatch")
    require(doc.get("contract_id") == "RAFAELIA-MISSION-RUNTIME-EVIDENCE-CLOSURE-V1", "contract id mismatch")
    require(doc.get("claim_allowed") is False, "global claim_allowed must remain false")
    require(doc.get("scientific_claim_promotion") is False, "scientific claim promotion must remain false")
    require(doc.get("weight_training_authorized") is False, "weight training must remain unauthorized")
    require(doc.get("source_completion_state") == "SOURCE_READY_EXTERNAL_GATES_ONLY", "source completion state mismatch")
    require(doc.get("terminal_program_state_when_required_evidence_is_valid") == "FINISHED_WITH_EXTERNAL_GATES", "terminal state mismatch")

    invariants = set(doc.get("invariants") or [])
    require(REQUIRED_INVARIANTS <= invariants, "required invariants are missing")

    gates = doc.get("evidence_gates")
    require(isinstance(gates, list), "evidence_gates must be a list")
    require(len(gates) == 8, "exactly eight evidence gates are required")

    gate_map: dict[str, dict[str, Any]] = {}
    for gate in gates:
        require(isinstance(gate, dict), "every gate must be an object")
        gate_id = gate.get("gate_id")
        require(isinstance(gate_id, str) and gate_id, "gate_id missing")
        require(gate_id not in gate_map, f"duplicate gate_id: {gate_id}")
        require(str(gate.get("current_state", "")).startswith("TOKEN_VAZIO"), f"source gate must remain TOKEN_VAZIO: {gate_id}")
        require(isinstance(gate.get("required_fields"), list) and gate["required_fields"], f"required_fields missing: {gate_id}")
        require(isinstance(gate.get("invalid_substitutes"), list) and gate["invalid_substitutes"], f"invalid_substitutes missing: {gate_id}")
        require(nonempty(gate.get("promotion_scope")), f"promotion_scope missing: {gate_id}")
        gate_map[gate_id] = gate

    require(set(gate_map) == REQUIRED_GATE_IDS, "gate id set mismatch")

    non_gates = doc.get("non_gates") or {}
    training = non_gates.get("model_weight_training") or {}
    science = non_gates.get("scientific_claim_promotion") or {}
    require(training.get("state") == "NOT_AUTHORIZED", "training must stay outside mission closure")
    require(science.get("state") == "BLOCKED", "scientific claim promotion must stay blocked")

    completion = doc.get("completion_rule") or {}
    require(completion.get("required_gate_count") == 8, "completion rule gate count mismatch")
    require(completion.get("all_required_gates_must_pass") is True, "all gates must pass")
    require(completion.get("partial_pass_promotes_global_completion") is False, "partial completion must not promote")
    require(completion.get("terminal_state") == "FINISHED_WITH_EXTERNAL_GATES", "completion terminal state mismatch")
    require(completion.get("claim_allowed_after_terminal_state") is False, "terminal runtime closure must not promote scientific claims")
    return gate_map


def walk_forbidden_keys(value: Any, forbidden: set[str], prefix: str = "") -> None:
    if isinstance(value, dict):
        for key, item in value.items():
            key_l = str(key).lower()
            require(key_l not in forbidden, f"forbidden secret-bearing field at {prefix}{key}")
            walk_forbidden_keys(item, forbidden, prefix=f"{prefix}{key}.")
    elif isinstance(value, list):
        for index, item in enumerate(value):
            walk_forbidden_keys(item, forbidden, prefix=f"{prefix}{index}.")


def validate_gate_evidence(gate: dict[str, Any], evidence: dict[str, Any]) -> None:
    gate_id = gate["gate_id"]
    require(isinstance(evidence, dict), f"evidence must be an object: {gate_id}")

    for field in gate["required_fields"]:
        require(field in evidence and nonempty(evidence[field]), f"missing/empty {gate_id}.{field}")

    for field, expected in (gate.get("required_values") or {}).items():
        require(evidence.get(field) == expected, f"required value mismatch {gate_id}.{field}")

    forbidden = {str(x).lower() for x in gate.get("forbidden_fields") or []}
    if forbidden:
        walk_forbidden_keys(evidence, forbidden)

    for field, value in evidence.items():
        field_l = field.lower()
        if field_l.endswith("_sha256"):
            require(isinstance(value, str) and SHA256_RE.fullmatch(value) is not None, f"invalid SHA-256 {gate_id}.{field}")
        if field_l.endswith("_utc"):
            parse_time(value, f"{gate_id}.{field}")

    if gate_id == "exact_multi_repository_runtime_execution":
        heads = evidence.get("repository_heads")
        require(isinstance(heads, dict), "repository_heads must be an object")
        required_keys = set(gate.get("required_repository_keys") or [])
        require(required_keys <= set(heads), "repository_heads missing required producers")
        for repo_name in required_keys:
            sha = heads[repo_name]
            require(isinstance(sha, str) and SHA1_RE.fullmatch(sha) is not None, f"invalid exact head for {repo_name}")


def validate_receipt(contract: dict[str, Any], gate_map: dict[str, dict[str, Any]], receipt: dict[str, Any]) -> None:
    require(receipt.get("schema") == EXPECTED_RECEIPT_SCHEMA, "receipt schema mismatch")
    require(receipt.get("contract_id") == contract.get("contract_id"), "receipt contract binding mismatch")
    require(receipt.get("claim_allowed") is False, "receipt may not promote global claim_allowed")
    require(receipt.get("scientific_claim_promotion") is False, "receipt may not promote scientific claims")
    require(receipt.get("weight_training_authorized") is False, "receipt may not authorize weight training")

    gates = receipt.get("gates")
    require(isinstance(gates, dict), "receipt gates must be an object")
    require(set(gates) == set(gate_map), "terminal receipt must contain exactly all required gates")

    for gate_id, gate in gate_map.items():
        validate_gate_evidence(gate, gates[gate_id])

    require(receipt.get("terminal_state") == "FINISHED_WITH_EXTERNAL_GATES", "terminal receipt state mismatch")


def synthetic_receipt(contract: dict[str, Any], gate_map: dict[str, dict[str, Any]]) -> dict[str, Any]:
    now = "2026-09-07T12:00:00Z"
    h256 = "a" * 64
    heads = {name: "b" * 40 for name in gate_map["exact_multi_repository_runtime_execution"]["required_repository_keys"]}
    return {
        "schema": EXPECTED_RECEIPT_SCHEMA,
        "contract_id": contract["contract_id"],
        "claim_allowed": False,
        "scientific_claim_promotion": False,
        "weight_training_authorized": False,
        "terminal_state": "FINISHED_WITH_EXTERNAL_GATES",
        "gates": {
            "physical_android_termux_execution": {
                "device_receipt_json": "synthetic-self-test.json",
                "device_receipt_sha256": h256,
                "apk_sha256": h256,
                "device_fingerprint_or_privacy_preserving_id": "SELF_TEST_ONLY",
                "capture_time_utc": now,
                "result": "PASS"
            },
            "exact_multi_repository_runtime_execution": {
                "repository_heads": heads,
                "execution_plan_id": "SELF_TEST_PLAN",
                "authorized_action_ids": ["SELF_TEST_ACTION"],
                "execution_result_ids": ["SELF_TEST_RESULT"],
                "provenance_receipt_sha256": h256,
                "runtime_start_utc": now,
                "runtime_end_utc": now,
                "result": "PASS"
            },
            "remote_network_identity": {
                "identity_mechanism": "SELF_TEST_CHALLENGE",
                "peer_identifier": "SELF_TEST_PEER",
                "challenge_or_session_id": "SELF_TEST_SESSION",
                "verification_result": "PASS",
                "evidence_digest": h256,
                "observed_at_utc": now
            },
            "provider_or_legal_authorization": {
                "authority_name": "SELF_TEST_AUTHORITY",
                "decision_reference": "SELF_TEST_REFERENCE",
                "scope": "SELF_TEST_SCOPE",
                "decision": "AUTHORIZED",
                "effective_at_utc": now,
                "evidence_digest_or_provider_reference": "SELF_TEST_PROVIDER_REFERENCE"
            },
            "live_default_branch_ruleset": {
                "repository": "SELF_TEST_REPOSITORY",
                "default_branch": "main",
                "provider_observation_id": "SELF_TEST_OBSERVATION",
                "ruleset_or_protection_snapshot_digest": h256,
                "observed_at_utc": now,
                "result": "PASS"
            },
            "server_merge_enforcement": {
                "repository": "SELF_TEST_REPOSITORY",
                "branch": "main",
                "enforcement_observation_id": "SELF_TEST_ENFORCEMENT",
                "server_result": "PASS",
                "observed_at_utc": now,
                "evidence_digest": h256
            },
            "manual_promotion_decision": {
                "decision_id": "SELF_TEST_DECISION",
                "authority": "SELF_TEST_AUTHORITY",
                "scope": "SELF_TEST_SCOPE",
                "decision": "APPROVED",
                "decided_at_utc": now,
                "evidence_digest_or_provider_reference": "SELF_TEST_PROVIDER_REFERENCE"
            },
            "codescan_credentialed_analysis": {
                "analysis_run_id": "SELF_TEST_ANALYSIS",
                "analysis_result": "PASS",
                "report_digest": h256,
                "analyzed_ref": "b" * 40,
                "observed_at_utc": now
            }
        }
    }


def run_self_test(contract: dict[str, Any], gate_map: dict[str, dict[str, Any]]) -> None:
    valid = synthetic_receipt(contract, gate_map)
    validate_receipt(contract, gate_map, valid)

    missing_gate = copy.deepcopy(valid)
    missing_gate["gates"].pop("physical_android_termux_execution")
    try:
        validate_receipt(contract, gate_map, missing_gate)
    except ValidationError:
        pass
    else:
        raise ValidationError("self-test failed: missing gate was accepted")

    wrong_result = copy.deepcopy(valid)
    wrong_result["gates"]["physical_android_termux_execution"]["result"] = "TOKEN_VAZIO_DEVICE"
    try:
        validate_receipt(contract, gate_map, wrong_result)
    except ValidationError:
        pass
    else:
        raise ValidationError("self-test failed: TOKEN_VAZIO device result was accepted")

    secret_leak = copy.deepcopy(valid)
    secret_leak["gates"]["codescan_credentialed_analysis"]["token"] = "MUST_NOT_PERSIST"
    try:
        validate_receipt(contract, gate_map, secret_leak)
    except ValidationError:
        pass
    else:
        raise ValidationError("self-test failed: secret-bearing key was accepted")

    print("PASS synthetic positive + negative fail-closed tests (no external evidence promoted)")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--contract", type=Path, default=DEFAULT_CONTRACT)
    parser.add_argument("--receipt", type=Path)
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args()

    try:
        contract = load_json(args.contract)
        gate_map = validate_contract(contract)
        if args.self_test:
            run_self_test(contract, gate_map)
        if args.receipt:
            receipt = load_json(args.receipt)
            validate_receipt(contract, gate_map, receipt)
            print("PASS terminal runtime receipt shape: FINISHED_WITH_EXTERNAL_GATES")
            print("claim_allowed=false scientific_claim_promotion=false weight_training_authorized=false")
        elif not args.self_test:
            print("PASS source closure contract: SOURCE_READY_EXTERNAL_GATES_ONLY")
            print("external/device/runtime gates remain TOKEN_VAZIO until real evidence is supplied")
        return 0
    except ValidationError as exc:
        print(f"FAIL_CLOSED: {exc}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
