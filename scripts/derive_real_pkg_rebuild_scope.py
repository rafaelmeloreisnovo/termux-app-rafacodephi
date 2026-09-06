#!/usr/bin/env python3
"""Derive the minimal package rebuild scope from prefix-audit + ownership receipts."""
from __future__ import annotations

import argparse
import hashlib
import json
from collections import defaultdict
from pathlib import Path

SCHEMA = "rafcodephi-real-pkg-rebuild-scope/v1"
AUDIT_SCHEMA = "rafcodephi-real-pkg-prefix-audit/v1"
OWNERSHIP_SCHEMA = "rafcodephi-real-pkg-package-ownership/v1"


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def derive(audit: dict, ownership: dict) -> dict:
    if audit.get("schema") != AUDIT_SCHEMA:
        raise ValueError(f"unexpected audit schema: {audit.get('schema')}")
    if ownership.get("schema") != OWNERSHIP_SCHEMA:
        raise ValueError(f"unexpected ownership schema: {ownership.get('schema')}")

    by_package: dict[str, list[str]] = defaultdict(list)
    package_meta: dict[str, dict] = {}
    unattributed: list[str] = []
    owner_entries = ownership.get("entries", {})

    for entry in sorted(set(audit.get("binary_risk_entries", []))):
        owner = owner_entries.get(entry)
        if not isinstance(owner, dict) or not owner.get("package"):
            unattributed.append(entry)
            continue
        package = str(owner["package"])
        by_package[package].append(entry)
        package_meta[package] = owner

    packages = []
    for package in sorted(by_package):
        meta = package_meta[package]
        entries = sorted(by_package[package])
        packages.append({
            "package": package,
            "version": meta.get("version", "UNAVAILABLE"),
            "filename": meta.get("filename", "UNAVAILABLE"),
            "deb_sha256": meta.get("deb_sha256", "UNAVAILABLE"),
            "binary_risk_entry_count": len(entries),
            "binary_risk_entries": entries,
        })

    scope_complete = not unattributed
    return {
        "schema": SCHEMA,
        "state": "READY_TO_REBUILD" if scope_complete else "BLOCKED_OWNERSHIP_GAPS",
        "canonical_prefix": audit.get("canonical_prefix"),
        "audit_state": audit.get("state"),
        "audit_reason": audit.get("reason"),
        "binary_risk_entry_count": audit.get("binary_risk_entry_count", 0),
        "package_rebuild_count": len(packages),
        "packages": packages,
        "attributed_binary_risk_entry_count": sum(len(v) for v in by_package.values()),
        "unattributed_binary_risk_entry_count": len(unattributed),
        "unattributed_binary_risk_entries": unattributed,
        "scope_complete": scope_complete,
        "claim_allowed_structural_real_pkg": False,
        "claim_allowed_device_runtime": False,
        "release_allowed": False,
        "device_validation": "TOKEN_VAZIO",
        "next_required_action": (
            "REBUILD_PACKAGE_SET_FROM_SOURCE_FOR_RAFCODEPHI_PREFIX"
            if scope_complete
            else "RESOLVE_PACKAGE_OWNERSHIP_GAPS_BEFORE_REBUILD"
        ),
        "invariant": (
            "Package attribution narrows rebuild scope; it does not prove that packages were rebuilt, "
            "that the bootstrap is runtime-safe, or that release is allowed."
        ),
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--audit", type=Path, required=True)
    parser.add_argument("--ownership", type=Path, required=True)
    parser.add_argument("--out", type=Path, required=True)
    args = parser.parse_args()

    if not args.audit.is_file() or not args.ownership.is_file():
        raise SystemExit("audit/ownership input missing")
    audit = json.loads(args.audit.read_text(encoding="utf-8"))
    ownership = json.loads(args.ownership.read_text(encoding="utf-8"))
    payload = derive(audit, ownership)
    payload["prefix_audit_sha256"] = sha256(args.audit)
    payload["ownership_sha256"] = sha256(args.ownership)

    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_text(json.dumps(payload, ensure_ascii=False, sort_keys=True, indent=2) + "\n", encoding="utf-8")
    print(
        f"rebuild_scope={payload['state']} packages={payload['package_rebuild_count']} "
        f"attributed={payload['attributed_binary_risk_entry_count']} "
        f"unattributed={payload['unattributed_binary_risk_entry_count']}"
    )
    # A complete scope is successful evidence even though the prefix promotion
    # remains BLOCKED until the package rebuild itself is performed and audited.
    return 0 if payload["scope_complete"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
