#!/usr/bin/env python3
"""Fail-closed validator for direct static cross-domain inventory.

This validates repository-declared inventory structure only. It cannot promote
installed/loaded/executed/measured/reproduced or device-runtime claims.
"""
from __future__ import annotations
import argparse
import json
import re
from pathlib import Path

HEX40 = re.compile(r"^[0-9a-f]{40}$")
ALLOWED_GAP = {"OPEN", "TOKEN_VAZIO", "PARTIAL", "ERRATUM_REQUIRED", "CLOSED"}


def fail(msg: str) -> None:
    raise SystemExit(f"FAIL: {msg}")


def require(cond: bool, msg: str) -> None:
    if not cond:
        fail(msg)


def load(path: Path) -> dict:
    data = json.loads(path.read_text(encoding="utf-8"))
    require(isinstance(data, dict), "top-level JSON must be object")
    return data


def validate_source(source: dict, ctx: str) -> None:
    require(isinstance(source, dict), f"{ctx}: source must be object")
    require(isinstance(source.get("path"), str) and source["path"], f"{ctx}: source path missing")
    require(bool(HEX40.fullmatch(str(source.get("blob_sha", "")))), f"{ctx}: blob_sha must be 40 lowercase hex")


def validate(data: dict) -> dict:
    required = {
        "inventory_id", "generated_at", "base_commit", "evidence_class", "scope",
        "claim_allowed", "runtime_claim_promoted", "lifecycle_policy", "android_build",
        "modules", "native_declarations", "license_state", "document_drift", "gaps",
    }
    require(required <= set(data), f"missing keys: {sorted(required - set(data))}")
    require(data["claim_allowed"] is False, "claim_allowed must remain false")
    require(data["runtime_claim_promoted"] is False, "runtime_claim_promoted must remain false")
    require(data["evidence_class"] == "OBSERVED_STATIC", "evidence_class must be OBSERVED_STATIC")
    require(data["scope"] == "DIRECT_DECLARATIONS_ONLY", "scope must be DIRECT_DECLARATIONS_ONLY")
    require(bool(HEX40.fullmatch(str(data["base_commit"]))), "base_commit must be 40 lowercase hex")

    policy = data["lifecycle_policy"]
    require(policy.get("current_state") == "INVENTORIED", "only INVENTORIED is allowed in this static artifact")
    forbidden = {"INSTALLED", "LOADED", "RUNTIME_REACHABLE", "EXECUTED", "MEASURED", "REPRODUCED"}
    require(forbidden <= set(policy.get("not_implied", [])), "runtime non-implication boundary incomplete")

    build = data["android_build"]
    require(build.get("required_abis") == ["armeabi-v7a", "arm64-v8a"], "canonical ABI matrix drift")
    require(build.get("optional_abis") == [], "optional ABI set changed; audit explicitly")
    for i, src in enumerate(build.get("source", [])):
        validate_source(src, f"android_build.source[{i}]")

    modules = data["modules"]
    require(isinstance(modules, list) and modules, "modules must be non-empty list")
    ids = []
    for i, module in enumerate(modules):
        ctx = f"modules[{i}]"
        mid = module.get("id")
        require(isinstance(mid, str) and mid.startswith(":"), f"{ctx}: invalid module id")
        ids.append(mid)
        require(isinstance(module.get("direct_project_dependencies"), list), f"{ctx}: project deps must be list")
        ext = module.get("direct_external_dependencies")
        require(isinstance(ext, list), f"{ctx}: external deps must be list")
        require(len(ext) == len(set(ext)), f"{ctx}: duplicate direct external dependency")
        for j, src in enumerate(module.get("source", [])):
            validate_source(src, f"{ctx}.source[{j}]")
    require(len(ids) == len(set(ids)), "duplicate module ids")
    required_modules = {":app", ":termux-shared", ":terminal-emulator", ":terminal-view", ":rafaelia", ":rmr", ":loader"}
    require(required_modules <= set(ids), "canonical module inventory incomplete")
    for module in modules:
        unknown = set(module["direct_project_dependencies"]) - set(ids)
        require(not unknown, f"{module['id']}: unknown project dependencies {sorted(unknown)}")

    native = data["native_declarations"]
    require(isinstance(native, list), "native_declarations must be list")
    for i, item in enumerate(native):
        require(item.get("owner") in ids, f"native_declarations[{i}]: unknown owner")
        require(isinstance(item.get("modules"), list) and item["modules"], f"native_declarations[{i}]: modules missing")
        require(len(item["modules"]) == len(set(item["modules"])), f"native_declarations[{i}]: duplicate native module")
        validate_source(item.get("source"), f"native_declarations[{i}].source")

    licenses = data["license_state"]
    require(licenses.get("rafaelia_rmr", {}).get("state") == "CONFLICT",
            "RAFAELIA/RMR license-authority conflict must remain explicit until resolved")
    require(licenses.get("rafaelia_rmr", {}).get("promotion") == "BLOCKED_UNTIL_AUTHORITY_MAP",
            "license conflict must block promotion")

    drift = data["document_drift"]
    require(isinstance(drift, list) and drift, "document_drift must preserve detected drift")
    for i, item in enumerate(drift):
        require(item.get("state") == "ERRATUM_REQUIRED", f"document_drift[{i}]: drift must remain erratum-required")
        validate_source(item.get("document"), f"document_drift[{i}].document")

    gaps = data["gaps"]
    require(isinstance(gaps, list) and gaps, "gaps must be non-empty")
    gap_ids = []
    for i, gap in enumerate(gaps):
        gid = gap.get("id")
        require(isinstance(gid, str) and gid.startswith("GAP-"), f"gaps[{i}]: invalid id")
        gap_ids.append(gid)
        require(gap.get("state") in ALLOWED_GAP, f"gaps[{i}]: invalid state")
        require(isinstance(gap.get("next"), str) and gap["next"], f"gaps[{i}]: next action missing")
    require(len(gap_ids) == len(set(gap_ids)), "duplicate gap ids")

    return {
        "status": "PASS",
        "state": "PROVEN_STATIC_STRUCTURE",
        "modules": len(modules),
        "native_declaration_groups": len(native),
        "gaps": len(gaps),
        "claim_allowed": False,
        "runtime_claim_promoted": False,
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("inventory", type=Path)
    parser.add_argument("--json-report", type=Path)
    args = parser.parse_args()
    report = validate(load(args.inventory))
    if args.json_report:
        args.json_report.parent.mkdir(parents=True, exist_ok=True)
        args.json_report.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(report, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
