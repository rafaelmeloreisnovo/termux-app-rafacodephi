#!/usr/bin/env python3
"""ATLAS run -> pinned LLaMA provider -> local Governance decision.

No shell interpolation, no implicit provider trust, no automatic execution.
The local Termux governance documents remain the authority for capabilities.
"""
from __future__ import annotations

import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import subprocess
import sys
from datetime import datetime, timezone

ROOT = Path(__file__).resolve().parents[1]
CAPS_PATH = ROOT / "internal/governance/capabilities.json"
GATE_PATH = ROOT / "internal/governance/strict_readonly_gate_v1.py"
MAX_JSON = 1 << 20


def now():
    return datetime.now(timezone.utc).isoformat().replace("+00:00", "Z")


def canonical(obj) -> bytes:
    return (json.dumps(obj, ensure_ascii=False, sort_keys=True, separators=(",", ":")) + "\n").encode()


def sha256_bytes(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def sha256_path(path: Path) -> str:
    with path.open("rb") as handle:
        return hashlib.file_digest(handle, "sha256").hexdigest()


def read_json(path: Path):
    data = path.read_bytes()
    if len(data) > MAX_JSON:
        raise ValueError("oversized_json:" + path.name)
    return json.loads(data)


def require(cond, reason):
    if not cond:
        raise ValueError(reason)


def write_new(path: Path, value):
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("xb") as handle:
        handle.write(canonical(value))


def load_gate_module():
    spec = importlib.util.spec_from_file_location("strict_readonly_gate_v1", GATE_PATH)
    module = importlib.util.module_from_spec(spec)
    assert spec.loader is not None
    spec.loader.exec_module(module)
    return module


def validate_upstream(run_dir: Path):
    bundle = read_json(run_dir / "context_bundle.json")
    chunks = read_json(run_dir / "chunks.json")
    envelope = read_json(run_dir / "envelope.json")
    require(envelope.get("claim_allowed") is False, "upstream_claim_not_blocked")
    require(envelope.get("model_backend", {}).get("backend_id") == "LLAMA_LOCAL_RMRCTI", "upstream_backend_mismatch")
    refs = bundle.get("chunk_refs")
    require(isinstance(refs, list) and refs, "upstream_chunk_refs")
    chunk_ids = {item.get("chunk_id") for item in chunks if isinstance(item, dict)}
    expected_ids = {item.get("chunk_id") for item in refs if isinstance(item, dict)}
    require(None not in expected_ids and expected_ids and expected_ids <= chunk_ids, "upstream_chunk_identity")
    require(isinstance(bundle.get("working_directory"), str) and bundle["working_directory"], "upstream_working_directory")
    return bundle, expected_ids


def validate_provider(intent: dict, receipt: dict, bundle: dict, expected_ids: set[str]):
    require(receipt.get("schema") == "rafaelia.llama_intent_provider_receipt.v1", "provider_receipt_schema")
    require(receipt.get("claim_allowed") is False, "provider_claim_not_blocked")
    require(receipt.get("execution_granted") is False, "provider_execution_must_be_false")
    require(receipt.get("bundle_id") == bundle.get("bundle_id"), "provider_bundle_mismatch")
    require(receipt.get("status") == "PROPOSED_INTENT_GOVERNANCE_REQUIRED", "provider_status")
    require(intent.get("schema") == "rafaelia.intent.v1", "intent_schema")
    require(intent.get("source_bundle_id") == bundle.get("bundle_id"), "intent_bundle_mismatch")
    require(intent.get("target", {}).get("repo_path") == bundle.get("working_directory"), "intent_target_not_upstream")
    observed_ids = {item.get("chunk_id") for item in intent.get("evidence_refs", []) if isinstance(item, dict)}
    require(observed_ids == expected_ids, "intent_evidence_set_mismatch")
    require(receipt.get("intent_sha256") == sha256_bytes(canonical(intent)), "provider_intent_hash_mismatch")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--atlas-run", required=True, type=Path)
    parser.add_argument("--provider-script", required=True, type=Path)
    parser.add_argument("--provider-sha256", required=True)
    parser.add_argument("--output-dir", required=True, type=Path)
    parser.add_argument("--draft-json", type=Path)
    parser.add_argument("--llama-cli", type=Path)
    parser.add_argument("--model", type=Path)
    parser.add_argument("--operator-approved", action="store_true")
    args = parser.parse_args()

    run_dir = args.atlas_run.resolve(strict=True)
    provider = args.provider_script.resolve(strict=True)
    output = args.output_dir.resolve()
    require(not output.exists(), "output_exists")
    require(len(args.provider_sha256) == 64 and all(c in "0123456789abcdef" for c in args.provider_sha256), "provider_sha256_shape")
    require(sha256_path(provider) == args.provider_sha256, "provider_sha256_mismatch")
    require(not output.is_relative_to(run_dir), "output_inside_upstream_run")

    bundle, expected_ids = validate_upstream(run_dir)
    work = Path(bundle["working_directory"]).resolve()
    require(not output.is_relative_to(work), "output_inside_target_workdir")

    output.mkdir(parents=True, mode=0o700)
    provider_out = output / "provider"
    command = [sys.executable, str(provider), "--run-dir", str(run_dir), "--output-dir", str(provider_out)]
    if args.draft_json:
        require(not args.llama_cli and not args.model, "draft_and_live_mutually_exclusive")
        command += ["--draft-json", str(args.draft_json.resolve(strict=True))]
    elif args.llama_cli or args.model:
        require(args.llama_cli and args.model, "llama_cli_and_model_pair_required")
        command += ["--llama-cli", str(args.llama_cli.resolve(strict=True)), "--model", str(args.model.resolve(strict=True))]

    proc = subprocess.run(command, capture_output=True, timeout=150, check=False)
    require(len(proc.stdout) <= MAX_JSON and len(proc.stderr) <= MAX_JSON, "provider_log_limit")
    receipt = read_json(provider_out / "provider_receipt.json")

    base_receipt = {
        "schema": "rafaelia.atlas_llama_governance_bridge_receipt.v1",
        "observed_at": now(),
        "bundle_id": bundle["bundle_id"],
        "provider_sha256": args.provider_sha256,
        "capabilities_sha256": sha256_path(CAPS_PATH),
        "gate_sha256": sha256_path(GATE_PATH),
        "provider_exit_code": proc.returncode,
        "model_executed": bool(receipt.get("model_executed", False)),
        "execution_performed": False,
        "claim_allowed": False,
    }

    if proc.returncode == 2 and receipt.get("status") == "TOKEN_VAZIO":
        base_receipt.update({"status": "TOKEN_VAZIO", "decision": "human_review", "token_vazio": receipt.get("token_vazio", [])})
        write_new(output / "bridge_receipt.json", base_receipt)
        return 2
    if proc.returncode != 0:
        base_receipt.update({"status": "BLOCKED", "decision": "blocked", "reason": receipt.get("reason", "provider_failed")})
        write_new(output / "bridge_receipt.json", base_receipt)
        return 3

    intent = read_json(provider_out / "intent.json")
    validate_provider(intent, receipt, bundle, expected_ids)
    gate = load_gate_module()
    decision = gate.decide(intent, read_json(CAPS_PATH), operator_approved=args.operator_approved)

    governed = dict(intent)
    governed["execution_gate"] = decision["decision"]
    governed_constraints = list(governed.get("constraints", []))
    governed_constraints.append({"key": "local_governance_reason", "value": decision["reason"]})
    governed["constraints"] = governed_constraints
    write_new(output / "intent_governed.json", governed)

    base_receipt.update({
        "status": "GOVERNED",
        "decision": decision["decision"],
        "reason": decision["reason"],
        "provider_receipt_sha256": sha256_path(provider_out / "provider_receipt.json"),
        "provider_intent_sha256": sha256_path(provider_out / "intent.json"),
        "governed_intent_sha256": sha256_bytes(canonical(governed)),
        "operator_approved": bool(args.operator_approved),
        "execution_authorized": decision["decision"] == "allow",
    })
    write_new(output / "bridge_receipt.json", base_receipt)
    return {"allow": 0, "human_review": 4, "blocked": 3}[decision["decision"]]


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as exc:
        print(type(exc).__name__ + ":" + str(exc), file=sys.stderr)
        raise SystemExit(3)
