#!/usr/bin/env python3
"""Validate or explicitly install the additive master branch ruleset.

Default is offline validation. --verify reads provider state. --apply requires
a token, an exact verified canary SHA, and an explicit operator action.
No PUT, DELETE, rollback automation, or claim promotion is performed.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
import re
import sys
import urllib.error
import urllib.request
from datetime import datetime, timezone
from pathlib import Path

REPO = "rafaelmeloreisnovo/termux-app-rafacodephi"
BRANCH = "refactor/ci-workflow-dedup-review-20261111"
POLICY = Path("governance/provider/RAFCODEPHI_MASTER_RULESET_APPLY_V1.json")
API = f"https://api.github.com/repos/{REPO}"
CHECK = "ci-head-contract"


def validate_policy(data: dict) -> None:
    if data.get("name") != "RAFCODEPHI MASTER CI ENFORCEMENT V1":
        raise ValueError("unexpected ruleset identity")
    if data.get("target") != "branch" or data.get("enforcement") != "active":
        raise ValueError("ruleset must actively protect a branch")
    if data.get("bypass_actors") != []:
        raise ValueError("no bypass actors are authorized")
    if data.get("conditions") != {"ref_name": {"include": ["~DEFAULT_BRANCH"], "exclude": []}}:
        raise ValueError("ruleset must target default branch only")
    rules = data.get("rules")
    if not isinstance(rules, list) or len(rules) != 5:
        raise ValueError("exactly five canonical branch rules required")
    by_type = {rule.get("type"): rule for rule in rules}
    required = {"deletion", "non_fast_forward", "required_linear_history", "pull_request", "required_status_checks"}
    if set(by_type) != required:
        raise ValueError("required rule type missing or duplicate")
    if any("parameters" in by_type[name] for name in ("deletion", "non_fast_forward", "required_linear_history")):
        raise ValueError("unexpected base-rule parameter")
    pr = by_type["pull_request"].get("parameters", {})
    if pr.get("required_approving_review_count") != 0:
        raise ValueError("solo workflow uses 0 required human approvals")
    if pr.get("required_review_thread_resolution") is not True:
        raise ValueError("review threads must be resolved")
    if pr.get("allowed_merge_methods") != ["squash"]:
        raise ValueError("only enabled squash merge is permitted")
    status = by_type["required_status_checks"].get("parameters", {})
    if status.get("strict_required_status_checks_policy") is not True:
        raise ValueError("strict exact-head checks required")
    if status.get("required_status_checks") != [{"context": CHECK}]:
        raise ValueError("required check must be the unfiltered ci-head-contract job")


def matches_live(desired: dict, live: dict) -> bool:
    """Conservative structural match; ignore only provider-added response fields."""
    if any(live.get(key) != desired[key] for key in ("name", "target", "enforcement", "bypass_actors")):
        return False
    if live.get("conditions") != desired["conditions"]:
        return False
    actual_rules = live.get("rules") or []
    if len(actual_rules) != len(desired["rules"]):
        return False
    actual = {rule.get("type"): rule for rule in actual_rules}
    if set(actual) != {rule["type"] for rule in desired["rules"]}:
        return False
    for rule in desired["rules"]:
        observed = actual[rule["type"]]
        if rule.get("parameters"):
            for key, expected in rule["parameters"].items():
                if observed.get("parameters", {}).get(key) != expected:
                    return False
    return True


def api_call(method: str, path: str, token: str, body: dict | None = None):
    payload = json.dumps(body).encode("utf-8") if body is not None else None
    headers = {
        "Accept": "application/vnd.github+json",
        "User-Agent": "rafaelia-master-ruleset-v1",
        "X-GitHub-Api-Version": "2022-11-28",
    }
    if token:
        headers["Authorization"] = f"Bearer {token}"
    if payload is not None:
        headers["Content-Type"] = "application/json"
    req = urllib.request.Request(API + path, data=payload, headers=headers, method=method)
    with urllib.request.urlopen(req, timeout=20) as response:
        return json.load(response)


def existing_by_name(name: str, token: str):
    listed = api_call("GET", "/rulesets?per_page=100", token)
    matches = [item for item in listed if item.get("name") == name]
    if len(matches) > 1:
        raise RuntimeError("DUPLICATE_LIVE_RULESET_NAME")
    if not matches:
        return None
    return api_call("GET", f"/rulesets/{matches[0]['id']}", token)


def canary_head_ready(head: str, token: str) -> bool:
    if not re.fullmatch(r"[0-9a-f]{40}", head or ""):
        return False
    branch = api_call("GET", "/git/ref/heads/" + BRANCH, token)
    if (branch.get("object") or {}).get("sha") != head:
        return False
    checks = api_call("GET", f"/commits/{head}/check-runs?check_name={CHECK}", token)
    return any(
        run.get("name") == CHECK
        and run.get("head_sha") == head
        and run.get("status") == "completed"
        and run.get("conclusion") == "success"
        and (run.get("app") or {}).get("slug") == "github-actions"
        for run in checks.get("check_runs", [])
    )


def write_receipt(action: str, state: str, payload_hash: str, details: dict):
    stamp = datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%S%fZ")
    path = Path("reports") / f"master-ruleset-{stamp}.json"
    path.parent.mkdir(parents=True, exist_ok=True)
    envelope = {
        "schema": "rafaelia.master-ruleset-admin-receipt/v1",
        "action": action,
        "state": state,
        "source": POLICY.as_posix(),
        "payload_sha256": payload_hash,
        "details": details,
        "negative_provider_falsifier": "TOKEN_VAZIO_NOT_EXECUTED",
        "claim_allowed": False,
    }
    with path.open("x", encoding="utf-8") as stream:
        json.dump(envelope, stream, sort_keys=True, indent=2)
        stream.write("\n")
    print(json.dumps({"state": state, "receipt": str(path), "claim_allowed": False}))
    return 0 if state in {"READBACK_MATCH_SCOPED", "ALREADY_PRESENT_MATCH_SCOPED"} else 3


def main() -> int:
    parser = argparse.ArgumentParser()
    group = parser.add_mutually_exclusive_group()
    group.add_argument("--verify", action="store_true", help="provider GET; no write")
    group.add_argument("--apply", action="store_true", help="POST only after check success")
    parser.add_argument("--expected-head", default="", help="40-char canary SHA mandatory for --apply")
    args = parser.parse_args()
    data = json.loads(POLICY.read_text(encoding="utf-8"))
    validate_policy(data)
    digest = hashlib.sha256(POLICY.read_bytes()).hexdigest()
    if not (args.verify or args.apply):
        print(json.dumps({"state": "VALIDATED_LOCAL_ONLY", "sha256": digest, "claim_allowed": False}))
        return 0

    token = os.environ.get("GH_TOKEN") or os.environ.get("GITHUB_TOKEN") or ""
    if args.apply and not token:
        return write_receipt("APPLY", "BLOCKED_NO_ADMIN_TOKEN", digest, {})
    action = "APPLY" if args.apply else "VERIFY"
    try:
        current = existing_by_name(data["name"], token)
        if current is not None:
            if matches_live(data, current):
                return write_receipt(action, "ALREADY_PRESENT_MATCH_SCOPED",
                                     digest, {"ruleset_id": current["id"]})
            return write_receipt(action, "BLOCKED_EXISTING_RULESET_DRIFT",
                                 digest, {"ruleset_id": current.get("id", "TOKEN_VAZIO")})
        if not args.apply:
            return write_receipt(action, "BLOCKED_RULESET_NOT_INSTALLED", digest, {})
        if not canary_head_ready(args.expected_head, token):
            return write_receipt(action, "BLOCKED_EXACT_HEAD_CHECK_NOT_GREEN",
                                 digest, {"expected_head": args.expected_head or "TOKEN_VAZIO"})
        created = api_call("POST", "/rulesets", token, data)
        observed = api_call("GET", f"/rulesets/{created['id']}", token)
        return write_receipt(action, "READBACK_MATCH_SCOPED" if matches_live(data, observed)
                             else "BLOCKED_POST_READBACK_MISMATCH",
                             digest, {"ruleset_id": created["id"]})
    except (urllib.error.HTTPError, urllib.error.URLError, RuntimeError, ValueError, KeyError) as exc:
        # No token or HTTP body is disclosed in receipts.
        kind = type(exc).__name__
        return write_receipt(action, "BLOCKED_PROVIDER_ERROR", digest, {"error_type": kind})


if __name__ == "__main__":
    sys.exit(main())
