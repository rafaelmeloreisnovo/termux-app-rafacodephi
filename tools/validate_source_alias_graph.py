#!/usr/bin/env python3
"""Validate exact-source alias graph against the current checkout."""
from __future__ import annotations
import hashlib
import json
from collections import defaultdict
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
GRAPH = ROOT / "configs" / "source-alias-graph.v1.json"

def git_blob_sha1(path: Path) -> str:
    data = path.read_bytes()
    return hashlib.sha1(b"blob " + str(len(data)).encode("ascii") + b"\0" + data).hexdigest()

def current_groups(cfg: dict) -> dict[str, list[str]]:
    by: dict[str, list[str]] = defaultdict(list)
    exts = set(cfg["source_extensions"])
    for path in ROOT.rglob("*"):
        if not path.is_file() or path.suffix not in exts:
            continue
        if ".git" in path.parts or "build" in path.parts:
            continue
        by[git_blob_sha1(path)].append(path.relative_to(ROOT).as_posix())
    return {
        sha: sorted(paths)
        for sha, paths in by.items()
        if len(paths) > 1
    }

def main() -> int:
    cfg = json.loads(GRAPH.read_text(encoding="utf-8"))
    errors: list[str] = []
    now = current_groups(cfg)
    recorded = {g["git_blob_sha1"]: g for g in cfg["groups"]}

    if set(now) != set(recorded):
        errors.append("duplicate-group-set-drift")

    authority = tuple(cfg["authority_prefixes"])
    for sha, paths in now.items():
        item = recorded.get(sha)
        if item is None:
            continue
        if item["paths"] != paths:
            errors.append(f"{sha}:path-set-drift")
        if item["count"] != len(paths) or item["excess"] != len(paths) - 1:
            errors.append(f"{sha}:count-drift")
        candidates = [p for p in paths if p.startswith(authority)]
        canonical = item.get("canonical_path")
        status = item["status"]
        if len(candidates) == 1:
            if status != "CANONICAL_BOUND" or canonical != candidates[0]:
                errors.append(f"{sha}:canonical-binding-drift")
        elif len(candidates) > 1:
            if status != "CANONICAL_AMBIGUOUS" or canonical is not None:
                errors.append(f"{sha}:ambiguity-drift")
        else:
            if status != "TOKEN_VAZIO_AUTHORITY_BLOCKED" or canonical is not None:
                errors.append(f"{sha}:unbound-authority-drift")
        if item.get("claim_allowed") is not False:
            errors.append(f"{sha}:claim-boundary")

    excess = sum(len(paths) - 1 for paths in now.values())
    if cfg["duplicate_excess"] != excess:
        errors.append("duplicate-excess-drift")

    report = {
        "schema":"rafaelia.source-alias-graph-validation/v1",
        "status":"PASS" if not errors else "FAIL",
        "errors":errors,
        "duplicate_groups":len(now),
        "duplicate_excess":excess,
        "canonical_bound":sum(1 for g in recorded.values() if g["status"]=="CANONICAL_BOUND"),
        "canonical_ambiguous":sum(1 for g in recorded.values() if g["status"]=="CANONICAL_AMBIGUOUS"),
        "authority_blocked":sum(1 for g in recorded.values() if g["status"]=="TOKEN_VAZIO_AUTHORITY_BLOCKED"),
        "deletion_allowed":False,
        "claim_allowed":False,
    }
    print(json.dumps(report, indent=2, sort_keys=True))
    return 0 if not errors else 1

if __name__ == "__main__":
    raise SystemExit(main())
