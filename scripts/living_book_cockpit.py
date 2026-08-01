#!/usr/bin/env python3
"""Inspect a Living Book bundle in Termux without dispatching or executing it."""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
from typing import Any

SENSITIVE = {
    "seed", "summary", "messages", "conversation", "private_content",
    "credential", "credentials", "secret", "secrets", "token",
    "password", "cookie", "authorization"
}


def canonical_bytes(value: Any) -> bytes:
    return json.dumps(value, ensure_ascii=False, sort_keys=True, separators=(",", ":")).encode("utf-8")


def digests(value: Any) -> dict[str, str]:
    data = canonical_bytes(value)
    return {
        "sha256": hashlib.sha256(data).hexdigest(),
        "sha3_256": hashlib.sha3_256(data).hexdigest(),
        "blake2b_256": hashlib.blake2b(data, digest_size=32).hexdigest()
    }


def scan(value: Any, path: str = "$") -> list[str]:
    errors: list[str] = []
    if isinstance(value, dict):
        for key, child in value.items():
            if str(key).lower() in SENSITIVE:
                errors.append(f"sensitive key at {path}.{key}")
            errors.extend(scan(child, f"{path}.{key}"))
    elif isinstance(value, list):
        for index, child in enumerate(value):
            errors.extend(scan(child, f"{path}[{index}]"))
    return errors


def validate_contract(contract: dict[str, Any]) -> list[str]:
    errors: list[str] = []
    if contract.get("schema") != "termux-rafacodephi.living-book-cockpit/v1":
        errors.append("invalid cockpit schema")
    if contract.get("claim_allowed") is not False:
        errors.append("claim must remain blocked")
    if contract.get("runtime_execution_allowed") is not False:
        errors.append("runtime execution must remain blocked")
    forbidden = set(contract.get("forbidden_actions", []))
    required_forbidden = {
        "dispatch", "execute", "publish", "merge", "delete", "share",
        "sync_write", "shell_eval", "open_network_target"
    }
    for action in sorted(required_forbidden - forbidden):
        errors.append(f"missing forbidden action: {action}")
    allowed = set(contract.get("allowed_actions", []))
    if allowed & forbidden:
        errors.append("action cannot be both allowed and forbidden")
    for item in contract.get("menu", []):
        for action in item.get("actions", []):
            if action not in allowed:
                errors.append(f"menu action not allowed: {action}")
    return errors


def validate_bundle(bundle: dict[str, Any], contract: dict[str, Any]) -> list[str]:
    errors = scan(bundle)
    required = contract["bundle_requirements"]
    if bundle.get("schema") != required["schema"]:
        errors.append("bundle schema mismatch")
    if bundle.get("state") != required["state"]:
        errors.append("bundle state mismatch")
    body = {key: bundle[key] for key in ("source", "payload", "policy") if key in bundle}
    if bundle.get("integrity", {}).get("digests") != digests(body):
        errors.append("bundle digest mismatch")
    policy = bundle.get("policy", {})
    payload = bundle.get("payload", {})
    observed = {
        "transport_mode": policy.get("transport_mode"),
        "human_approval_state": policy.get("human_approval_state"),
        "dispatch_allowed": policy.get("dispatch_allowed"),
        "execution_allowed": policy.get("execution_allowed"),
        "publication_allowed": policy.get("publication_allowed"),
        "claim_allowed": policy.get("claim_allowed"),
        "network_target": policy.get("network_target"),
        "ir_embedded": payload.get("ir_embedded"),
        "private_source_embedded": payload.get("private_source_embedded")
    }
    for key, expected in required.items():
        if key not in ("schema", "state") and observed.get(key) != expected:
            errors.append(f"bundle requirement mismatch: {key}")
    return errors


def receipt(bundle: dict[str, Any], errors: list[str], device_profile: str) -> dict[str, Any]:
    return {
        "schema": "termux-rafacodephi.living-book-cockpit-receipt/v1",
        "event_id": "TOKEN_VAZIO_RUNTIME_EVENT_ID",
        "bundle_id": bundle.get("bundle_id", "TOKEN_VAZIO_BUNDLE_ID"),
        "bundle_digests": bundle.get("integrity", {}).get("digests", {}),
        "observed_at": "TOKEN_VAZIO_RUNTIME_TIMESTAMP",
        "device_profile": device_profile,
        "validation": {"state": "PASS_INSPECT_ONLY" if not errors else "FAIL_CLOSED", "errors": errors},
        "decision": "INSPECT_ONLY_NO_DISPATCH",
        "execution_performed": False,
        "claim_allowed": False,
        "TOKEN_VAZIO": ["physical_android_runtime", "exact_device_timestamp", "human_approval_digest"],
        "F_ok": "bundle inspected without dispatch" if not errors else "invalid bundle blocked",
        "F_gap": "Android UI wiring and physical receipt",
        "F_next": "human review of exact bundle digest"
    }


def render(contract: dict[str, Any], bundle: dict[str, Any], errors: list[str]) -> str:
    lines = ["RAFAELIA Middleware — Livro Vivo", "=" * 36]
    for index, item in enumerate(contract["menu"], 1):
        lines.append(f"{index:02d}. {item['label']}")
    lines += [
        "", f"bundle_id: {bundle.get('bundle_id', 'TOKEN_VAZIO')}",
        "state: " + ("PASS_INSPECT_ONLY" if not errors else "FAIL_CLOSED"),
        "dispatch: BLOCKED", "execution: BLOCKED", "publication: BLOCKED", "claim: BLOCKED"
    ]
    if errors:
        lines.extend(["errors:"] + [f"- {error}" for error in errors])
    return "\n".join(lines)


def load(path: Path) -> dict[str, Any]:
    with path.open("r", encoding="utf-8") as handle:
        value = json.load(handle)
    if not isinstance(value, dict):
        raise ValueError("top-level JSON must be object")
    return value


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--contract", type=Path, default=Path("configs/living-book-cockpit-v1.json"))
    parser.add_argument("--bundle", type=Path)
    parser.add_argument("--device-profile", default="TOKEN_VAZIO_DEVICE_PROFILE")
    parser.add_argument("--receipt-out", type=Path)
    args = parser.parse_args()
    try:
        contract = load(args.contract)
        errors = validate_contract(contract)
        bundle = {} if args.bundle is None else load(args.bundle)
        if args.bundle is not None:
            errors += validate_bundle(bundle, contract)
        print(render(contract, bundle, errors))
        if args.receipt_out:
            args.receipt_out.parent.mkdir(parents=True, exist_ok=True)
            args.receipt_out.write_text(
                json.dumps(receipt(bundle, errors, args.device_profile), ensure_ascii=False, indent=2) + "\n",
                encoding="utf-8"
            )
    except (OSError, ValueError, json.JSONDecodeError) as exc:
        print(f"FAIL: {exc}")
        return 2
    return 1 if errors else 0


if __name__ == "__main__":
    raise SystemExit(main())
