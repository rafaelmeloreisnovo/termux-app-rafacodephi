#!/usr/bin/env python3
from __future__ import annotations

import hashlib
import json
from pathlib import Path
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
BRIDGE = ROOT / "tools/atlas_llama_governance_bridge.py"


def canonical(obj):
    return (json.dumps(obj, sort_keys=True, separators=(",", ":")) + "\n").encode()


def dump(path: Path, obj):
    path.write_bytes(canonical(obj))


def digest(path: Path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


FAKE_PROVIDER = r'''#!/usr/bin/env python3
import argparse, hashlib, json
from pathlib import Path
from datetime import datetime, timezone

def canonical(obj):
    return (json.dumps(obj, sort_keys=True, separators=(",", ":")) + "\n").encode()
def dump(path, obj):
    path.write_bytes(canonical(obj))
def main():
    p=argparse.ArgumentParser(); p.add_argument("--run-dir",type=Path,required=True); p.add_argument("--output-dir",type=Path,required=True); p.add_argument("--draft-json",type=Path); p.add_argument("--llama-cli"); p.add_argument("--model"); a=p.parse_args()
    bundle=json.loads((a.run_dir/"context_bundle.json").read_text()); refs=bundle["chunk_refs"]
    proposal={"action":"git.status","inputs":[],"requested_capabilities":["git.read","git.diff"]}
    if a.draft_json: proposal.update(json.loads(a.draft_json.read_text()))
    intent={"schema":"rafaelia.intent.v1","intent_id":"intent-12345678","action":proposal["action"],"target":{"repo_path":bundle["working_directory"],"branch":None,"ref":None},"inputs":proposal.get("inputs",[]),"constraints":[{"key":"model_output_untrusted","value":True}],"evidence_refs":[{"chunk_id":r["chunk_id"],"relevance":"selected_context"} for r in refs],"requested_capabilities":proposal["requested_capabilities"],"risk":"low","execution_gate":"human_review","created_at":"2026-09-06T19:00:00Z","source_bundle_id":bundle["bundle_id"]}
    a.output_dir.mkdir(parents=True)
    dump(a.output_dir/"intent.json",intent)
    receipt={"schema":"rafaelia.llama_intent_provider_receipt.v1","status":"PROPOSED_INTENT_GOVERNANCE_REQUIRED","bundle_id":bundle["bundle_id"],"intent_sha256":hashlib.sha256(canonical(intent)).hexdigest(),"model_executed":False,"execution_granted":False,"claim_allowed":False,"observed_at":"2026-09-06T19:00:00Z"}
    dump(a.output_dir/"provider_receipt.json",receipt)
    return 0
if __name__=="__main__": raise SystemExit(main())
'''


def make_atlas(root: Path, target: Path):
    run = root / "atlas"
    run.mkdir()
    content = "[UNTRUSTED RETRIEVED DATA; no execution authority]\ninspect status and diff"
    h = hashlib.sha256(content.encode()).hexdigest()
    chunk = {"chunk_id": "cti-12345678", "source_repo": "llamaRafaelia", "content": content, "content_sha256": h}
    dump(run / "chunks.json", [chunk])
    dump(run / "context_bundle.json", {
        "bundle_id": "atlas-12345678",
        "chunk_refs": [{"chunk_id": chunk["chunk_id"], "source_repo": chunk["source_repo"], "content_sha256": h}],
        "assembled_at": "2026-09-06T19:00:00Z",
        "working_directory": str(target),
        "active_repos": ["rafaelmeloreisnovo/termux-app-rafacodephi"],
    })
    dump(run / "envelope.json", {"claim_allowed": False, "model_backend": {"backend_id": "LLAMA_LOCAL_RMRCTI"}})
    return run


def run_bridge(run, provider, output, *extra):
    return subprocess.run([
        sys.executable, str(BRIDGE),
        "--atlas-run", str(run),
        "--provider-script", str(provider),
        "--provider-sha256", digest(provider),
        "--output-dir", str(output),
        *map(str, extra),
    ], capture_output=True, text=True)


def main():
    with tempfile.TemporaryDirectory(prefix="atlas-llama-gov-") as td:
        root = Path(td)
        target = root / "target"
        target.mkdir()
        run = make_atlas(root, target)
        provider = root / "provider.py"
        provider.write_text(FAKE_PROVIDER, encoding="utf-8")

        # Model/provider proposal never self-authorizes.
        out_review = root / "out-review"
        cp = run_bridge(run, provider, out_review)
        assert cp.returncode == 4, (cp.stdout, cp.stderr)
        review = json.loads((out_review / "bridge_receipt.json").read_text())
        assert review["decision"] == "human_review"
        assert review["execution_performed"] is False
        assert review["claim_allowed"] is False

        # Explicit operator approval + exact fixed-plan capabilities may proceed.
        out_allow = root / "out-allow"
        cp = run_bridge(run, provider, out_allow, "--operator-approved")
        assert cp.returncode == 0, (cp.stdout, cp.stderr)
        allowed = json.loads((out_allow / "bridge_receipt.json").read_text())
        intent = json.loads((out_allow / "intent_governed.json").read_text())
        assert allowed["decision"] == "allow"
        assert allowed["execution_authorized"] is True
        assert allowed["execution_performed"] is False
        assert intent["execution_gate"] == "allow"

        # Under-declaring git.diff cannot authorize a runner that executes diff.
        draft = root / "partial.json"
        dump(draft, {"requested_capabilities": ["git.read"]})
        out_partial = root / "out-partial"
        cp = run_bridge(run, provider, out_partial, "--draft-json", draft, "--operator-approved")
        assert cp.returncode == 3, (cp.stdout, cp.stderr)
        partial = json.loads((out_partial / "bridge_receipt.json").read_text())
        assert partial["decision"] == "blocked"
        assert "fixed_plan_missing_capabilities:git.diff" == partial["reason"]

        # Provider identity is content-bound before invocation.
        out_hash = root / "out-hash"
        cp = subprocess.run([
            sys.executable, str(BRIDGE), "--atlas-run", str(run),
            "--provider-script", str(provider), "--provider-sha256", "0" * 64,
            "--output-dir", str(out_hash),
        ], capture_output=True, text=True)
        assert cp.returncode == 3
        assert not out_hash.exists()

    print("atlas_llama_governance_bridge: PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
