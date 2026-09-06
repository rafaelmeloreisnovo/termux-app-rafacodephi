#!/usr/bin/env python3
"""Strict governance for the fixed Vertical Slice V1 read-only plan.

The legacy runner executes BOTH git provenance/status reads and git diff --stat.
Therefore an intent may enter that fixed plan only when it requests exactly the
capabilities required by the commands the runner will actually execute.

This module never treats model output as approval. `operator_approved` is an
explicit runtime input supplied by the caller after a human/user decision.
"""
from __future__ import annotations

import argparse
import json
from pathlib import Path

REQUIRED_FIXED_PLAN_CAPS = frozenset({"git.read", "git.diff"})
VALID_GATES = {"allow", "sandbox_only", "human_review", "blocked"}
VALID_RISKS = {"low", "medium", "high", "critical"}


def load_json(path: Path):
    with path.open(encoding="utf-8") as handle:
        return json.load(handle)


def decide(intent: dict, capabilities_doc: dict, operator_approved: bool = False) -> dict:
    """Return a deterministic policy decision without executing anything."""
    allowed = {item["id"]: item for item in capabilities_doc.get("capabilities", [])}
    requested = intent.get("requested_capabilities", [])

    if not isinstance(requested, list) or not all(isinstance(x, str) for x in requested):
        return _result("blocked", "invalid_capability_shape", requested)
    if len(requested) != len(set(requested)):
        return _result("blocked", "duplicate_capability", requested)

    gate = intent.get("execution_gate")
    risk = intent.get("risk")
    if gate not in VALID_GATES:
        return _result("blocked", "invalid_execution_gate", requested)
    if risk not in VALID_RISKS:
        return _result("blocked", "invalid_risk", requested)
    if gate == "blocked" or risk == "critical":
        return _result("blocked", "explicit_block_or_critical_risk", requested)

    unknown = sorted(set(requested) - set(allowed))
    if unknown:
        return _result("blocked", "unknown_capabilities:" + ",".join(unknown), requested)

    requested_set = set(requested)
    missing = sorted(REQUIRED_FIXED_PLAN_CAPS - requested_set)
    extra = sorted(requested_set - REQUIRED_FIXED_PLAN_CAPS)
    if missing:
        return _result("blocked", "fixed_plan_missing_capabilities:" + ",".join(missing), requested)
    if extra:
        return _result("blocked", "fixed_plan_extra_capabilities:" + ",".join(extra), requested)

    # Re-check classifications from the local authority document. A future
    # policy downgrade must fail closed even if the intent shape is unchanged.
    non_allow = sorted(cap for cap in requested if allowed[cap].get("classification") != "allow")
    if non_allow:
        return _result("blocked", "local_classification_not_allow:" + ",".join(non_allow), requested)

    if risk == "high":
        return _result("human_review", "high_risk", requested)
    if not operator_approved:
        return _result("human_review", "operator_approval_required", requested)

    return _result("allow", "exact_fixed_plan_capabilities_and_operator_approval", requested)


def _result(decision: str, reason: str, requested) -> dict:
    return {
        "schema": "rafaelia.strict_readonly_gate.v1",
        "decision": decision,
        "reason": reason,
        "requested_capabilities": requested if isinstance(requested, list) else [],
        "required_capabilities": sorted(REQUIRED_FIXED_PLAN_CAPS),
        "execution_performed": False,
        "claim_allowed": False,
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--intent", required=True, type=Path)
    parser.add_argument("--capabilities", required=True, type=Path)
    parser.add_argument("--operator-approved", action="store_true")
    args = parser.parse_args()

    result = decide(load_json(args.intent), load_json(args.capabilities), args.operator_approved)
    print(json.dumps(result, sort_keys=True, separators=(",", ":")))
    return {"allow": 0, "human_review": 4, "blocked": 3}[result["decision"]]


if __name__ == "__main__":
    raise SystemExit(main())
