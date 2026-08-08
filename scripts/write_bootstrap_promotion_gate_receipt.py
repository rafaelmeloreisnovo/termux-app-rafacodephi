#!/usr/bin/env python3
"""Bind a real-pkg prefix audit to the exact raw bootstrap artifact.

A BLOCKED receipt is a successful observation of a blocker, not a successful
real-pkg candidate. Runtime/release claims always remain closed here.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
from datetime import datetime, timezone
from pathlib import Path

SCHEMA = "rafcodephi-bootstrap-promotion-gate-receipt/v1"


def sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as fh:
        for chunk in iter(lambda: fh.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--audit", type=Path, required=True)
    parser.add_argument("--bootstrap", type=Path, required=True)
    parser.add_argument("--out", type=Path, required=True)
    args = parser.parse_args()

    if not args.audit.is_file() or not args.bootstrap.is_file():
        raise SystemExit("promotion receipt inputs missing")
    audit = json.loads(args.audit.read_text(encoding="utf-8"))
    if audit.get("schema") != "rafcodephi-real-pkg-prefix-audit/v1":
        raise SystemExit(f"unexpected audit schema: {audit.get('schema')}")
    state = audit.get("state")
    if state not in {"PASS", "BLOCKED", "FAIL"}:
        raise SystemExit(f"unexpected audit state: {state}")

    receipt = {
        "schema": SCHEMA,
        "generated_at_utc": datetime.now(timezone.utc).isoformat().replace("+00:00", "Z"),
        "git_sha": os.environ.get("GITHUB_SHA", "UNAVAILABLE"),
        "package_name": "com.termux.rafacodephi",
        "target_arch": "arm",
        "promotion_gate_state": state,
        "promotion_gate_reason": audit.get("reason", "UNAVAILABLE"),
        "prefix_audit_path": str(args.audit),
        "prefix_audit_sha256": sha256(args.audit),
        "raw_bootstrap_path": str(args.bootstrap),
        "raw_bootstrap_sha256": sha256(args.bootstrap),
        "binary_risk_entry_count": audit.get("binary_risk_entry_count", 0),
        "text_risk_entry_count": audit.get("text_risk_entry_count", 0),
        "next_required_action": audit.get("next_required_action", "UNAVAILABLE"),
        "token_vazio": audit.get("token_vazio", []),
        "claim_allowed_real_pkg_profile_materialization": state == "PASS",
        "claim_allowed_apk_candidate": False,
        "claim_allowed_device_runtime": False,
        "claim_allowed_pkg_runtime": False,
        "release_allowed": False,
        "device_validation": "TOKEN_VAZIO",
        "invariant": (
            "A BLOCKED promotion gate is valid evidence that unsafe prefix promotion was prevented. "
            "It is not evidence that real pkg is usable in the RAFCODEPhi runtime."
        ),
    }
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_text(json.dumps(receipt, ensure_ascii=False, sort_keys=True, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(receipt, ensure_ascii=False, sort_keys=True, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
