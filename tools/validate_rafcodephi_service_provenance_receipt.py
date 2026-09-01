#!/usr/bin/env python3
"""Fail-closed validator for the RAFCODEPHI service provenance receipt.

The receipt describes a static source snapshot.  Passing this validator never
claims an APK, an Android service process, or a Vectras guest has run.  The
validator also hashes every source binding so a source change cannot silently
reuse an older receipt.
"""
from __future__ import annotations

import argparse
import copy
import hashlib
import json
import re
import sys
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_RECEIPT = ROOT / "docs/assurance/RAFCODEPHI_SERVICE_PROVENANCE_RECEIPT_20260901.v1.json"
REPOSITORY = "rafaelmeloreisnovo/termux-app-rafacodephi"
SHA256_RE = re.compile(r"^[a-f0-9]{64}$")
GIT_SHA_RE = re.compile(r"^[a-f0-9]{40}$")
GATE_STATES = {"PASS", "FAIL", "TOKEN_VAZIO", "NOT_APPLICABLE"}
EVIDENCE_CLASSES = {"MEASURED_LOCAL", "OBSERVED_STATIC", "DOCUMENTED_REMOTE", "DOCUMENTED", "TOKEN_VAZIO"}
REQUIRED_GATES = {
    "SVC_STATIC_BOOTSTRAP_GUARD",
    "SVC_MANIFEST_RUN_COMMAND_DECLARATION",
    "SVC_RUN_COMMAND_DISPATCH_LINK",
    "SVC_VECTRAS_STATIC_CONTRACT",
    "SVC_CURRENT_HEAD_CI",
    "SVC_PROVIDER_RULESET_ENFORCEMENT",
    "SVC_APK_BOOTSTRAP_IDENTITY",
    "SVC_PHYSICAL_DEVICE_RUNTIME",
    "SVC_VECTRAS_CONSUMER_E2E",
    "SVC_ARM32_ARM64_DEVICE_RECEIPTS",
}


class ValidationError(Exception):
    """A receipt violates a fail-closed invariant."""


def require(condition: bool, message: str) -> None:
    if not condition:
        raise ValidationError(message)


def exact_keys(value: Any, allowed: set[str], context: str, required: set[str] | None = None) -> dict[str, Any]:
    require(isinstance(value, dict), f"{context}: expected object")
    keys = set(value)
    unknown = sorted(keys - allowed)
    missing = sorted((required or set()) - keys)
    require(not unknown, f"{context}: unknown fields: {unknown}")
    require(not missing, f"{context}: missing fields: {missing}")
    return value


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for block in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def load_receipt(path: Path) -> dict[str, Any]:
    try:
        value = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        raise ValidationError(f"cannot load receipt {path}: {exc}") from exc
    require(isinstance(value, dict), "receipt: top-level value must be an object")
    return value


def validate_source_bindings(bindings: Any, root: Path, verify_hashes: bool) -> int:
    require(isinstance(bindings, list) and bindings, "source_bindings: must be a non-empty list")
    seen_paths: set[str] = set()
    for index, binding in enumerate(bindings):
        ctx = f"source_bindings[{index}]"
        item = exact_keys(
            binding,
            {"path", "git_blob_sha", "sha256", "evidence_class", "purpose"},
            ctx,
            {"path", "git_blob_sha", "sha256", "evidence_class", "purpose"},
        )
        path = item["path"]
        require(isinstance(path, str) and path and not path.startswith("/"), f"{ctx}.path: must be a repository-relative path")
        require(".." not in Path(path).parts, f"{ctx}.path: must not escape repository root")
        require(path not in seen_paths, f"{ctx}.path: duplicate source binding")
        seen_paths.add(path)
        require(isinstance(item["git_blob_sha"], str) and GIT_SHA_RE.fullmatch(item["git_blob_sha"]) is not None, f"{ctx}.git_blob_sha: must be lowercase 40-hex")
        require(isinstance(item["sha256"], str) and SHA256_RE.fullmatch(item["sha256"]) is not None, f"{ctx}.sha256: must be lowercase 64-hex")
        require(item["evidence_class"] in EVIDENCE_CLASSES, f"{ctx}.evidence_class: invalid class")
        require(isinstance(item["purpose"], str) and item["purpose"], f"{ctx}.purpose: required")
        if verify_hashes:
            resolved = root / path
            require(resolved.is_file(), f"{ctx}.path: missing source file {path}")
            observed = sha256_file(resolved)
            require(observed == item["sha256"], f"{ctx}.sha256: source drift for {path}; expected {item['sha256']}, observed {observed}")
    return len(seen_paths)


def validate_gates(gates: Any) -> dict[str, int]:
    require(isinstance(gates, list) and gates, "gates: must be a non-empty list")
    ids: set[str] = set()
    counts = {state: 0 for state in GATE_STATES}
    for index, gate in enumerate(gates):
        ctx = f"gates[{index}]"
        item = exact_keys(
            gate,
            {"id", "state", "evidence_class", "evidence", "falsifier", "next_verifiable_action"},
            ctx,
            {"id", "state", "evidence_class", "evidence", "falsifier", "next_verifiable_action"},
        )
        require(isinstance(item["id"], str) and item["id"], f"{ctx}.id: required")
        require(item["id"] not in ids, f"{ctx}.id: duplicate gate")
        ids.add(item["id"])
        require(item["state"] in GATE_STATES, f"{ctx}.state: invalid state")
        counts[item["state"]] += 1
        require(item["evidence_class"] in EVIDENCE_CLASSES, f"{ctx}.evidence_class: invalid class")
        require(isinstance(item["evidence"], list) and item["evidence"], f"{ctx}.evidence: must be non-empty list")
        require(all(isinstance(entry, str) and entry for entry in item["evidence"]), f"{ctx}.evidence: entries must be non-empty strings")
        require(isinstance(item["falsifier"], str) and item["falsifier"], f"{ctx}.falsifier: required")
        require(isinstance(item["next_verifiable_action"], str) and item["next_verifiable_action"], f"{ctx}.next_verifiable_action: required")
    missing = sorted(REQUIRED_GATES - ids)
    require(not missing, f"gates: missing required gate(s): {missing}")
    return counts


def validate_gaps(gaps: Any) -> int:
    require(isinstance(gaps, list) and gaps, "gaps: must be a non-empty list")
    ids: set[str] = set()
    for index, gap in enumerate(gaps):
        ctx = f"gaps[{index}]"
        item = exact_keys(gap, {"id", "priority", "state", "closure"}, ctx, {"id", "priority", "state", "closure"})
        require(isinstance(item["id"], str) and item["id"], f"{ctx}.id: required")
        require(item["id"] not in ids, f"{ctx}.id: duplicate gap")
        ids.add(item["id"])
        require(item["priority"] in {"P0", "P1", "P2", "P3"}, f"{ctx}.priority: invalid priority")
        require(item["state"] in {"OPEN", "PARTIAL", "TOKEN_VAZIO"}, f"{ctx}.state: must preserve an open uncertainty")
        require(isinstance(item["closure"], str) and item["closure"], f"{ctx}.closure: required")
    return len(ids)


def validate_receipt(data: dict[str, Any], root: Path, verify_hashes: bool = True) -> dict[str, Any]:
    allowed = {
        "schema", "receipt_id", "parent_receipt", "observed_at", "repository", "source", "scope", "authority",
        "evidence_boundary", "source_bindings", "gates", "gaps", "controls", "decision", "r3", "rollback",
    }
    required = allowed
    receipt = exact_keys(data, allowed, "receipt", required)
    require(receipt["schema"] == "rafaelia.rafcodephi.service-provenance-receipt.v1", "receipt.schema: unsupported schema")
    require(isinstance(receipt["receipt_id"], str) and receipt["receipt_id"].startswith("RAFCODEPHI-SVC-PROVENANCE-"), "receipt.receipt_id: invalid id")
    require(isinstance(receipt["parent_receipt"], str) and receipt["parent_receipt"], "receipt.parent_receipt: required")
    require(isinstance(receipt["observed_at"], str) and receipt["observed_at"].endswith("Z"), "receipt.observed_at: UTC timestamp required")
    require(receipt["repository"] == REPOSITORY, "receipt.repository: wrong authority repository")

    source = exact_keys(receipt["source"], {"ref", "commit", "tree"}, "source", {"ref", "commit", "tree"})
    require(source["ref"] == "master", "source.ref: expected master baseline")
    require(isinstance(source["commit"], str) and GIT_SHA_RE.fullmatch(source["commit"]) is not None, "source.commit: invalid SHA")
    require(isinstance(source["tree"], str) and GIT_SHA_RE.fullmatch(source["tree"]) is not None, "source.tree: invalid SHA")

    exact_keys(receipt["scope"], {"service_components", "excluded_claims"}, "scope", {"service_components", "excluded_claims"})
    require(isinstance(receipt["scope"]["service_components"], list) and receipt["scope"]["service_components"], "scope.service_components: required")
    require(isinstance(receipt["scope"]["excluded_claims"], list) and receipt["scope"]["excluded_claims"], "scope.excluded_claims: required")

    exact_keys(receipt["authority"], {"producer", "federated_router", "executor", "evidence_kernel"}, "authority", {"producer", "federated_router", "executor", "evidence_kernel"})
    exact_keys(receipt["evidence_boundary"], {"literal", "contextual", "latent"}, "evidence_boundary", {"literal", "contextual", "latent"})
    for key in ("literal", "contextual", "latent"):
        values = receipt["evidence_boundary"][key]
        require(isinstance(values, list) and values and all(isinstance(value, str) and value for value in values), f"evidence_boundary.{key}: non-empty strings required")

    binding_count = validate_source_bindings(receipt["source_bindings"], root, verify_hashes)
    gate_counts = validate_gates(receipt["gates"])
    gap_count = validate_gaps(receipt["gaps"])

    controls = exact_keys(receipt["controls"], {"public_payload", "prohibited_public_payloads", "graph"}, "controls", {"public_payload", "prohibited_public_payloads", "graph"})
    require(controls["public_payload"] == "MINIMUM_METADATA_PLUS_HASH_OR_TYPED_REFERENCE", "controls.public_payload: must be privacy-minimized")
    require(isinstance(controls["prohibited_public_payloads"], list) and controls["prohibited_public_payloads"], "controls.prohibited_public_payloads: required")
    require(controls["graph"] == "docs/assurance/RAFCODEPHI_SERVICE_DEPENDENCY_GRAPH_20260901.v1.json", "controls.graph: unexpected graph path")

    decision = exact_keys(receipt["decision"], {"claim_allowed", "release_allowed", "promotion", "falsifier"}, "decision", {"claim_allowed", "release_allowed", "promotion", "falsifier"})
    require(decision["claim_allowed"] is False, "decision.claim_allowed: static receipt cannot promote a runtime claim")
    require(decision["release_allowed"] is False, "decision.release_allowed: incomplete gates must block release")
    require(decision["promotion"] == "BLOCKED", "decision.promotion: expected BLOCKED")
    require(isinstance(decision["falsifier"], str) and decision["falsifier"], "decision.falsifier: required")
    require(gate_counts["FAIL"] + gate_counts["TOKEN_VAZIO"] > 0, "decision: a fail-closed receipt requires an unresolved gate")

    r3 = exact_keys(receipt["r3"], {"F_ok", "F_gap", "F_next"}, "r3", {"F_ok", "F_gap", "F_next"})
    for key in ("F_ok", "F_gap", "F_next"):
        require(isinstance(r3[key], list) and r3[key] and all(isinstance(value, str) and value for value in r3[key]), f"r3.{key}: required")

    rollback = exact_keys(receipt["rollback"], {"available", "procedure"}, "rollback", {"available", "procedure"})
    require(rollback["available"] is True, "rollback.available: receipt-only mutation must be reversible")
    require(isinstance(rollback["procedure"], str) and rollback["procedure"], "rollback.procedure: required")

    return {
        "status": "PASS",
        "state": "DOCUMENTED_STATIC_SNAPSHOT",
        "claim_allowed": False,
        "release_allowed": False,
        "source_bindings": binding_count,
        "gates": gate_counts,
        "open_gaps": gap_count,
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("receipt", nargs="?", type=Path, default=DEFAULT_RECEIPT)
    parser.add_argument("--skip-source-hash-check", action="store_true")
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args()

    try:
        receipt = load_receipt(args.receipt)
        report = validate_receipt(receipt, ROOT, verify_hashes=not args.skip_source_hash_check)
        if args.self_test:
            invalid = copy.deepcopy(receipt)
            invalid["decision"]["claim_allowed"] = True
            rejected = False
            try:
                validate_receipt(invalid, ROOT, verify_hashes=False)
            except ValidationError:
                rejected = True
            require(rejected, "self-test: promoted runtime claim was accepted")
            report["negative_promotion_rejected"] = True
        print(json.dumps(report, ensure_ascii=False, sort_keys=True, indent=2))
        return 0
    except ValidationError as exc:
        print(json.dumps({"status": "FAIL", "state": "REJECTED_FAIL_CLOSED", "claim_allowed": False, "detail": str(exc)}, ensure_ascii=False, sort_keys=True, indent=2), file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
