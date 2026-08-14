#!/usr/bin/env python3
"""Fail-closed validator for the RAFAELIA cross-domain dependency graph.

This validator deliberately separates structural validation from runtime proof.
A graph can be structurally valid while `claim_allowed` remains false and one or
more evidence/promotion gates remain TOKEN_VAZIO.
"""
from __future__ import annotations

import argparse
import json
import re
import sys
from pathlib import Path
from typing import Any

ROOT = Path(__file__).resolve().parents[1]
DEFAULT_SCHEMA = ROOT / "docs" / "contracts" / "cross_domain_dependency_graph.schema.json"
DEFAULT_VALID = ROOT / "tests" / "fixtures" / "cross_domain_dependency_graph.valid.json"
DEFAULT_INVALID = ROOT / "tests" / "fixtures" / "cross_domain_dependency_graph.invalid_promotion.json"

DOMAINS = {
    "OS_SYSTEM", "OS_VENDOR", "VENDOR_DLKM", "APEX_MAINLINE", "HAL",
    "ANDROID_APP", "TERMUX_PACKAGE", "NATIVE_LIBRARY", "JNI", "TOOLCHAIN",
    "COMPILER", "RUNTIME", "FIRMWARE", "KERNEL", "DATASET", "OTHER",
}
LIFECYCLE = {
    "INVENTORIED", "INSTALLED", "LOADED", "RUNTIME_REACHABLE", "EXECUTED",
    "MEASURED", "REPRODUCED", "TOKEN_VAZIO",
}
RUNTIME_LIFECYCLE = {"LOADED", "RUNTIME_REACHABLE", "EXECUTED", "MEASURED", "REPRODUCED"}
RELATIONS = {
    "DEPENDS_ON", "ABI", "API", "LICENSE", "RUNTIME", "VERSION", "SECURITY",
    "PROVENANCE", "UPSTREAM", "COMPATIBILITY",
}
EVIDENCE_STATES = {"VERIFIED", "PARTIAL", "DECLARED", "INFERRED", "TOKEN_VAZIO"}
EVIDENCE_CLASSES = {
    "OBSERVED_RUNTIME", "OBSERVED_STATIC", "DECLARED_REPOSITORY", "DOCUMENTED",
    "INFERRED", "TOKEN_VAZIO",
}
SOURCE_TYPES = {
    "DEVICE_RECEIPT", "FILE", "GIT", "DRIVE", "SBOM", "PACKAGE_METADATA",
    "LICENSE_NOTICE", "UPSTREAM", "ADVISORY", "TEST", "DOCUMENTATION", "OTHER",
}
GATE_STATES = {"PASS", "FAIL", "TOKEN_VAZIO", "NOT_APPLICABLE"}
PRIORITIES = {"P0", "P1", "P2", "P3"}
GAP_STATES = {"OPEN", "TOKEN_VAZIO", "PARTIAL", "CLOSED", "ERRATUM_REQUIRED"}
PROMOTION_KEYS = {
    "compatibility", "tests", "license", "security", "provenance",
    "runtime_evidence", "promote_allowed",
}
TOP_KEYS = {"graph_id", "schema_version", "generated_at", "claim_allowed", "nodes", "edges", "gaps"}
SHA256_RE = re.compile(r"^[a-f0-9]{64}$")
TOKEN_RE = re.compile(r"^TOKEN_VAZIO(?:_[A-Z0-9_]+)?$")


class ValidationError(Exception):
    pass


def load_json(path: Path) -> dict[str, Any]:
    with path.open("r", encoding="utf-8") as handle:
        value = json.load(handle)
    if not isinstance(value, dict):
        raise ValidationError(f"{path}: top-level JSON must be an object")
    return value


def require(condition: bool, message: str) -> None:
    if not condition:
        raise ValidationError(message)


def exact_keys(value: dict[str, Any], allowed: set[str], context: str, required: set[str] | None = None) -> None:
    unknown = sorted(set(value) - allowed)
    require(not unknown, f"{context}: unknown fields: {unknown}")
    if required is not None:
        missing = sorted(required - set(value))
        require(not missing, f"{context}: missing fields: {missing}")


def validate_sha(value: Any, context: str) -> None:
    if value is None:
        return
    require(isinstance(value, str) and bool(value), f"{context}: sha256 must be non-empty string")
    require(bool(SHA256_RE.fullmatch(value) or TOKEN_RE.fullmatch(value)), f"{context}: sha256 must be 64 lowercase hex or TOKEN_VAZIO[_...] state")


def validate_provenance(records: Any, context: str) -> None:
    require(isinstance(records, list) and records, f"{context}: provenance must be a non-empty list")
    allowed = {"source_type", "source_pointer", "revision", "sha256", "observed_at", "evidence_class"}
    required = {"source_type", "source_pointer", "evidence_class"}
    for index, record in enumerate(records):
        item_ctx = f"{context}.provenance[{index}]"
        require(isinstance(record, dict), f"{item_ctx}: must be object")
        exact_keys(record, allowed, item_ctx, required)
        require(record["source_type"] in SOURCE_TYPES, f"{item_ctx}: invalid source_type={record['source_type']!r}")
        require(isinstance(record["source_pointer"], str) and record["source_pointer"], f"{item_ctx}: source_pointer required")
        require(record["evidence_class"] in EVIDENCE_CLASSES, f"{item_ctx}: invalid evidence_class={record['evidence_class']!r}")
        validate_sha(record.get("sha256"), item_ctx)


def validate_node(node: Any, index: int) -> str:
    ctx = f"nodes[{index}]"
    require(isinstance(node, dict), f"{ctx}: must be object")
    allowed = {"component_id", "name", "domain", "version", "lifecycle_state", "platform", "license", "provenance"}
    required = {"component_id", "name", "domain", "lifecycle_state", "provenance"}
    exact_keys(node, allowed, ctx, required)
    component_id = node["component_id"]
    require(isinstance(component_id, str) and len(component_id) >= 2, f"{ctx}: invalid component_id")
    require(isinstance(node["name"], str) and node["name"], f"{ctx}: name required")
    require(node["domain"] in DOMAINS, f"{ctx}: invalid domain={node['domain']!r}")
    require(node["lifecycle_state"] in LIFECYCLE, f"{ctx}: invalid lifecycle_state={node['lifecycle_state']!r}")
    validate_provenance(node["provenance"], ctx)

    if node["lifecycle_state"] in RUNTIME_LIFECYCLE:
        runtime_evidence = any(item.get("evidence_class") == "OBSERVED_RUNTIME" for item in node["provenance"])
        require(runtime_evidence, f"{ctx}: lifecycle_state={node['lifecycle_state']} requires OBSERVED_RUNTIME provenance")

    platform = node.get("platform")
    if platform is not None:
        require(isinstance(platform, dict), f"{ctx}.platform: must be object")
        exact_keys(platform, {"os_family", "os_version", "api_level", "abi", "kernel", "device_model"}, f"{ctx}.platform")
        if "abi" in platform:
            require(isinstance(platform["abi"], list), f"{ctx}.platform.abi: must be list")
            require(len(platform["abi"]) == len(set(platform["abi"])), f"{ctx}.platform.abi: duplicate ABI")

    license_info = node.get("license")
    if license_info is not None:
        require(isinstance(license_info, dict), f"{ctx}.license: must be object")
        exact_keys(license_info, {"state", "spdx_id", "license_ref", "obligations"}, f"{ctx}.license", {"state"})
        require(license_info["state"] in {"VERIFIED", "DECLARED", "CONFLICT", "TOKEN_VAZIO"}, f"{ctx}.license: invalid state")

    return component_id


def validate_promotion_gate(gate: Any, context: str) -> None:
    require(isinstance(gate, dict), f"{context}: must be object")
    exact_keys(gate, PROMOTION_KEYS, context, PROMOTION_KEYS)
    for key in PROMOTION_KEYS - {"promote_allowed"}:
        require(gate[key] in GATE_STATES, f"{context}.{key}: invalid gate state={gate[key]!r}")
    require(isinstance(gate["promote_allowed"], bool), f"{context}.promote_allowed: must be boolean")
    if gate["promote_allowed"]:
        failures = {key: gate[key] for key in PROMOTION_KEYS - {"promote_allowed"} if gate[key] != "PASS"}
        require(not failures, f"{context}: promotion is fail-closed; all gates must PASS, got {failures}")


def validate_edge(edge: Any, index: int, node_ids: set[str]) -> str:
    ctx = f"edges[{index}]"
    require(isinstance(edge, dict), f"{ctx}: must be object")
    allowed = {"edge_id", "from", "to", "relation", "evidence_state", "promotion_gate", "provenance"}
    required = allowed
    exact_keys(edge, allowed, ctx, required)
    edge_id = edge["edge_id"]
    require(isinstance(edge_id, str) and len(edge_id) >= 4, f"{ctx}: invalid edge_id")
    require(edge["from"] in node_ids, f"{ctx}: unknown from node={edge['from']!r}")
    require(edge["to"] in node_ids, f"{ctx}: unknown to node={edge['to']!r}")
    require(edge["relation"] in RELATIONS, f"{ctx}: invalid relation={edge['relation']!r}")
    require(edge["evidence_state"] in EVIDENCE_STATES, f"{ctx}: invalid evidence_state={edge['evidence_state']!r}")
    validate_promotion_gate(edge["promotion_gate"], f"{ctx}.promotion_gate")
    validate_provenance(edge["provenance"], ctx)
    return edge_id


def validate_gap(gap: Any, index: int) -> str:
    ctx = f"gaps[{index}]"
    require(isinstance(gap, dict), f"{ctx}: must be object")
    allowed = {"gap_id", "priority", "state", "description", "next_verifiable_action", "closed_by"}
    required = {"gap_id", "priority", "state", "description", "next_verifiable_action"}
    exact_keys(gap, allowed, ctx, required)
    gap_id = gap["gap_id"]
    require(isinstance(gap_id, str) and len(gap_id) >= 4, f"{ctx}: invalid gap_id")
    require(gap["priority"] in PRIORITIES, f"{ctx}: invalid priority={gap['priority']!r}")
    require(gap["state"] in GAP_STATES, f"{ctx}: invalid state={gap['state']!r}")
    require(isinstance(gap["description"], str) and gap["description"], f"{ctx}: description required")
    require(isinstance(gap["next_verifiable_action"], str) and gap["next_verifiable_action"], f"{ctx}: next_verifiable_action required")
    if gap["state"] == "CLOSED":
        require(isinstance(gap.get("closed_by"), str) and gap["closed_by"], f"{ctx}: CLOSED gap requires closed_by evidence pointer")
    return gap_id


def validate_graph(data: dict[str, Any]) -> dict[str, Any]:
    exact_keys(data, TOP_KEYS, "graph", TOP_KEYS)
    require(data["schema_version"] == "1.0.0", "graph.schema_version must be 1.0.0")
    require(data["claim_allowed"] is False, "graph.claim_allowed must remain false")
    require(isinstance(data["nodes"], list), "graph.nodes must be list")
    require(isinstance(data["edges"], list), "graph.edges must be list")
    require(isinstance(data["gaps"], list), "graph.gaps must be list")

    node_ids = [validate_node(node, index) for index, node in enumerate(data["nodes"])]
    require(len(node_ids) == len(set(node_ids)), "graph.nodes contains duplicate component_id")
    node_set = set(node_ids)

    edge_ids = [validate_edge(edge, index, node_set) for index, edge in enumerate(data["edges"])]
    require(len(edge_ids) == len(set(edge_ids)), "graph.edges contains duplicate edge_id")

    gap_ids = [validate_gap(gap, index) for index, gap in enumerate(data["gaps"])]
    require(len(gap_ids) == len(set(gap_ids)), "graph.gaps contains duplicate gap_id")

    return {
        "status": "PASS",
        "state": "PROVEN_STRUCTURAL",
        "nodes": len(node_ids),
        "edges": len(edge_ids),
        "gaps": len(gap_ids),
        "claim_allowed": False,
        "runtime_claim_promoted": False,
    }


def optional_jsonschema_check(data: dict[str, Any], schema_path: Path) -> str:
    try:
        import jsonschema  # type: ignore
    except ImportError:
        return "SKIPPED_JSONSCHEMA_NOT_INSTALLED"
    schema = load_json(schema_path)
    jsonschema.validate(data, schema)
    return "PASS"


def validate_path(path: Path, schema_path: Path) -> dict[str, Any]:
    data = load_json(path)
    report = validate_graph(data)
    report["jsonschema"] = optional_jsonschema_check(data, schema_path)
    report["input"] = str(path)
    return report


def self_test(schema_path: Path, valid_path: Path, invalid_path: Path) -> dict[str, Any]:
    valid_report = validate_path(valid_path, schema_path)
    invalid_rejected = False
    invalid_reason = ""
    try:
        validate_path(invalid_path, schema_path)
    except Exception as exc:  # expected rejection path
        invalid_rejected = True
        invalid_reason = str(exc)
    require(invalid_rejected, "negative fixture was unexpectedly accepted")
    return {
        "status": "PASS",
        "state": "PROVEN_STRUCTURAL_SELF_TEST",
        "valid_fixture": valid_report,
        "negative_fixture_rejected": True,
        "negative_reason": invalid_reason,
        "claim_allowed": False,
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("graph", nargs="?", type=Path)
    parser.add_argument("--schema", type=Path, default=DEFAULT_SCHEMA)
    parser.add_argument("--self-test", action="store_true")
    parser.add_argument("--valid-fixture", type=Path, default=DEFAULT_VALID)
    parser.add_argument("--invalid-fixture", type=Path, default=DEFAULT_INVALID)
    parser.add_argument("--json-report", type=Path)
    args = parser.parse_args()

    try:
        if args.self_test:
            report = self_test(args.schema, args.valid_fixture, args.invalid_fixture)
        else:
            require(args.graph is not None, "graph path is required unless --self-test is used")
            report = validate_path(args.graph, args.schema)
    except Exception as exc:
        report = {"status": "FAIL", "state": "REJECTED_FAIL_CLOSED", "detail": str(exc), "claim_allowed": False}
        print(json.dumps(report, ensure_ascii=False, sort_keys=True, indent=2), file=sys.stderr)
        if args.json_report:
            args.json_report.parent.mkdir(parents=True, exist_ok=True)
            args.json_report.write_text(json.dumps(report, ensure_ascii=False, sort_keys=True, indent=2) + "\n", encoding="utf-8")
        return 2

    print(json.dumps(report, ensure_ascii=False, sort_keys=True, indent=2))
    if args.json_report:
        args.json_report.parent.mkdir(parents=True, exist_ok=True)
        args.json_report.write_text(json.dumps(report, ensure_ascii=False, sort_keys=True, indent=2) + "\n", encoding="utf-8")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
