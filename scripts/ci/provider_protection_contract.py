#!/usr/bin/env python3
"""Evaluate the live GitHub default-branch ruleset against the canonical target.

SOURCE != PROVIDER_STATE != OBSERVATION != EVIDENCE != CLAIM.

The desired state lives in governance/provider/PROVIDER_RULESET_TARGET_20260831.v1.json.
This module is stdlib-only so the same evaluator can run in GitHub Actions and in
offline unit tests. A failing live configuration still emits a machine-readable
receipt before returning a non-zero exit code.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
import sys
import urllib.error
import urllib.request
from pathlib import Path
from typing import Any

DEFAULT_TARGET = Path("governance/provider/PROVIDER_RULESET_TARGET_20260831.v1.json")
DEFAULT_JSON = Path("reports/provider-protection-receipt.json")
DEFAULT_MD = Path("reports/provider-protection-receipt.md")
API_VERSION = "2022-11-28"
TOKEN_VAZIO = "TOKEN_VAZIO"


def canonical_sha256(value: Any) -> str:
    payload = json.dumps(value, sort_keys=True, separators=(",", ":"), ensure_ascii=False)
    return hashlib.sha256(payload.encode("utf-8")).hexdigest()


def file_sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as fh:
        for chunk in iter(lambda: fh.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()


def load_target(path: Path) -> dict[str, Any]:
    data = json.loads(path.read_text(encoding="utf-8"))
    if data.get("schema") != "rafaelia.provider_ruleset_target/v1":
        raise ValueError(f"unsupported target schema: {data.get('schema')!r}")
    if not isinstance(data.get("target"), dict):
        raise ValueError("target contract missing object: target")
    return data


def github_get(url: str, token: str) -> Any:
    headers = {
        "Accept": "application/vnd.github+json",
        "User-Agent": "rafaelia-provider-protection-contract-v2",
        "X-GitHub-Api-Version": API_VERSION,
    }
    if token:
        headers["Authorization"] = f"Bearer {token}"
    req = urllib.request.Request(url, headers=headers)
    with urllib.request.urlopen(req, timeout=20) as response:
        return json.load(response)


def fetch_live_rulesets(repository: str, default_branch: str, token: str) -> list[dict[str, Any]]:
    base = f"https://api.github.com/repos/{repository}"
    listed = github_get(base + "/rulesets", token)
    applicable: list[dict[str, Any]] = []

    for item in listed:
        if item.get("enforcement") != "active":
            continue
        detail = github_get(base + f"/rulesets/{item['id']}", token)
        cond = ((detail.get("conditions") or {}).get("ref_name") or {})
        includes = set(cond.get("include") or [])
        if "~DEFAULT_BRANCH" in includes or f"refs/heads/{default_branch}" in includes:
            applicable.append(detail)

    return applicable


def _rules_by_type(rulesets: list[dict[str, Any]]) -> dict[str, list[dict[str, Any]]]:
    by_type: dict[str, list[dict[str, Any]]] = {}
    for rs in rulesets:
        for rule in rs.get("rules") or []:
            rule_type = rule.get("type")
            if rule_type:
                by_type.setdefault(rule_type, []).append(rule)
    return by_type


def _always_bypass_integrations(rulesets: list[dict[str, Any]]) -> list[int]:
    ids: set[int] = set()
    for rs in rulesets:
        for actor in rs.get("bypass_actors") or []:
            if actor.get("actor_type") == "Integration" and actor.get("bypass_mode") == "always":
                actor_id = actor.get("actor_id")
                if isinstance(actor_id, int):
                    ids.add(actor_id)
    return sorted(ids)


def _check_pull_request(
    target: dict[str, Any],
    observed_rules: list[dict[str, Any]],
) -> tuple[bool, dict[str, Any], list[dict[str, Any]]]:
    expected = target.get("pull_request") or {}
    comparable_keys = (
        "required_approving_review_count",
        "require_code_owner_review",
        "required_review_thread_resolution",
        "dismiss_stale_reviews_on_push",
        "require_last_push_approval",
        "allowed_merge_methods",
    )
    expected_cmp = {k: expected[k] for k in comparable_keys if k in expected}
    observations: list[dict[str, Any]] = []

    for rule in observed_rules:
        params = rule.get("parameters") or {}
        mismatches = {
            key: {"expected": value, "observed": params.get(key, TOKEN_VAZIO)}
            for key, value in expected_cmp.items()
            if params.get(key, TOKEN_VAZIO) != value
        }
        observations.append(
            {
                "parameters": {key: params.get(key, TOKEN_VAZIO) for key in expected_cmp},
                "mismatches": mismatches,
            }
        )
        if not mismatches:
            return True, expected_cmp, observations

    return False, expected_cmp, observations


def _check_required_status(
    target: dict[str, Any],
    observed_rules: list[dict[str, Any]],
) -> tuple[bool, dict[str, Any], list[dict[str, Any]]]:
    expected = target.get("required_status_checks") or {}
    expected_contexts = set(expected.get("contexts") or [])
    expected_strict = bool(expected.get("strict_required_status_checks_policy"))
    observations: list[dict[str, Any]] = []

    for rule in observed_rules:
        params = rule.get("parameters") or {}
        contexts = {
            item.get("context")
            for item in (params.get("required_status_checks") or [])
            if isinstance(item, dict) and item.get("context")
        }
        strict = params.get("strict_required_status_checks_policy")
        missing_contexts = sorted(expected_contexts - contexts)
        obs = {
            "contexts": sorted(contexts),
            "strict_required_status_checks_policy": strict
            if isinstance(strict, bool)
            else TOKEN_VAZIO,
            "missing_contexts": missing_contexts,
        }
        observations.append(obs)
        if not missing_contexts and strict is expected_strict:
            return True, {
                "contexts": sorted(expected_contexts),
                "strict_required_status_checks_policy": expected_strict,
            }, observations

    return False, {
        "contexts": sorted(expected_contexts),
        "strict_required_status_checks_policy": expected_strict,
    }, observations


def evaluate(
    contract: dict[str, Any],
    rulesets: list[dict[str, Any]],
    *,
    target_path: str,
    target_sha256: str,
    repository: str,
    default_branch: str,
) -> dict[str, Any]:
    target = contract["target"]
    by_type = _rules_by_type(rulesets)

    required_types = sorted(
        set(target.get("preserve_rules") or []) | set(target.get("add_rules") or [])
    )
    observed_types = sorted(by_type)
    missing_types = sorted(set(required_types) - set(observed_types))

    pr_ok, pr_expected, pr_observed = _check_pull_request(
        target, by_type.get("pull_request", [])
    )
    status_ok, status_expected, status_observed = _check_required_status(
        target, by_type.get("required_status_checks", [])
    )

    bypass_ids = _always_bypass_integrations(rulesets)
    bypass_state = (
        "NONE_OBSERVED"
        if not bypass_ids
        else "TOKEN_VAZIO_PENDING_IDENTITY_AND_JUSTIFICATION"
    )

    failures: list[dict[str, Any]] = []
    if missing_types:
        failures.append(
            {
                "code": "MISSING_RULE_TYPES",
                "expected": required_types,
                "observed": observed_types,
                "missing": missing_types,
            }
        )
    if not pr_ok:
        failures.append(
            {
                "code": "PULL_REQUEST_POLICY_MISMATCH",
                "expected": pr_expected,
                "observed_candidates": pr_observed,
            }
        )
    if not status_ok:
        failures.append(
            {
                "code": "REQUIRED_STATUS_CHECKS_MISMATCH",
                "expected": status_expected,
                "observed_candidates": status_observed,
            }
        )

    gate = "PASS" if not failures else "FAIL"
    live_digest = canonical_sha256(rulesets)

    remediation = {
        "authority": "GitHub provider ruleset administration",
        "apply_state": "TOKEN_VAZIO_NOT_APPLIED_BY_THIS_EVALUATOR",
        "operations": [],
    }
    if "required_status_checks" in missing_types or not status_ok:
        remediation["operations"].append(
            {
                "kind": "ENSURE_REQUIRED_STATUS_CHECKS",
                "desired": status_expected,
            }
        )
    if not pr_ok:
        remediation["operations"].append(
            {
                "kind": "ALIGN_PULL_REQUEST_POLICY",
                "desired": pr_expected,
            }
        )

    return {
        "schema": "rafaelia.provider_protection_receipt/v2",
        "state": "OBSERVED",
        "gate": gate,
        "repository": repository,
        "default_branch": default_branch,
        "target": {
            "path": target_path,
            "sha256": target_sha256,
            "schema": contract["schema"],
        },
        "live_observation": {
            "ruleset_ids": [rs.get("id") for rs in rulesets],
            "ruleset_names": [rs.get("name") for rs in rulesets],
            "digest_sha256": live_digest,
            "rule_types": observed_types,
            "always_bypass_integrations": bypass_ids,
            "bypass_identity_state": bypass_state,
        },
        "checks": {
            "required_rule_types": {
                "pass": not missing_types,
                "expected": required_types,
                "observed": observed_types,
                "missing": missing_types,
            },
            "pull_request": {
                "pass": pr_ok,
                "expected": pr_expected,
                "observed_candidates": pr_observed,
            },
            "required_status_checks": {
                "pass": status_ok,
                "expected": status_expected,
                "observed_candidates": status_observed,
            },
        },
        "failures": failures,
        "remediation": remediation,
        "claim_allowed": False,
        "provider_apply_state": contract.get("provider_apply_state", TOKEN_VAZIO),
        "invariants": [
            "TARGET_FILE != LIVE_PROVIDER_STATE",
            "WORKFLOW_PASS != PROVIDER_ENFORCEMENT",
            "TOKEN_VAZIO != PASS",
        ],
    }


def render_markdown(receipt: dict[str, Any]) -> str:
    checks = receipt["checks"]
    lines = [
        "# Provider Protection Receipt V2",
        "",
        f"- gate: **{receipt['gate']}**",
        f"- repository: `{receipt['repository']}`",
        f"- default branch: `{receipt['default_branch']}`",
        f"- target SHA-256: `{receipt['target']['sha256']}`",
        f"- live digest SHA-256: `{receipt['live_observation']['digest_sha256']}`",
        f"- live ruleset IDs: `{receipt['live_observation']['ruleset_ids']}`",
        f"- bypass identity state: **{receipt['live_observation']['bypass_identity_state']}**",
        "",
        "## Gates",
        "",
        "| Gate | State |",
        "|---|---|",
        f"| required rule types | {'PASS' if checks['required_rule_types']['pass'] else 'FAIL'} |",
        f"| pull request policy | {'PASS' if checks['pull_request']['pass'] else 'FAIL'} |",
        f"| required status checks | {'PASS' if checks['required_status_checks']['pass'] else 'FAIL'} |",
        "",
    ]

    if receipt["failures"]:
        lines.extend(["## Failures", ""])
        for failure in receipt["failures"]:
            lines.append(f"- **{failure['code']}**")
        lines.append("")

    lines.extend(
        [
            "## Boundary",
            "",
            "- `TARGET_FILE != LIVE_PROVIDER_STATE`",
            "- `WORKFLOW_PASS != PROVIDER_ENFORCEMENT`",
            "- `claim_allowed=false`",
            f"- provider apply state: **{receipt['remediation']['apply_state']}**",
            "",
        ]
    )
    return "\n".join(lines)


def write_receipt(receipt: dict[str, Any], json_path: Path, md_path: Path) -> None:
    json_path.parent.mkdir(parents=True, exist_ok=True)
    md_path.parent.mkdir(parents=True, exist_ok=True)
    json_path.write_text(
        json.dumps(receipt, indent=2, sort_keys=True, ensure_ascii=False) + "\n",
        encoding="utf-8",
    )
    md_path.write_text(render_markdown(receipt), encoding="utf-8")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--target", type=Path, default=DEFAULT_TARGET)
    parser.add_argument("--repository", default=os.environ.get("GITHUB_REPOSITORY", ""))
    parser.add_argument("--default-branch", default="master")
    parser.add_argument("--token", default=os.environ.get("GH_TOKEN", ""))
    parser.add_argument("--json", type=Path, default=DEFAULT_JSON)
    parser.add_argument("--markdown", type=Path, default=DEFAULT_MD)
    return parser.parse_args()


def main() -> int:
    args = parse_args()

    if not args.repository:
        print("ERROR: repository is required (--repository or GITHUB_REPOSITORY)", file=sys.stderr)
        return 2

    try:
        contract = load_target(args.target)
        if contract.get("repository") != args.repository:
            raise ValueError(
                f"target repository mismatch: {contract.get('repository')} != {args.repository}"
            )
        if contract.get("default_branch") != args.default_branch:
            raise ValueError(
                f"target branch mismatch: {contract.get('default_branch')} != {args.default_branch}"
            )

        rulesets = fetch_live_rulesets(args.repository, args.default_branch, args.token)
        if not rulesets:
            receipt = {
                "schema": "rafaelia.provider_protection_receipt/v2",
                "state": "BLOCKED",
                "gate": "FAIL",
                "repository": args.repository,
                "default_branch": args.default_branch,
                "target": {
                    "path": str(args.target),
                    "sha256": file_sha256(args.target),
                    "schema": contract["schema"],
                },
                "live_observation": {
                    "ruleset_ids": [],
                    "digest_sha256": canonical_sha256([]),
                    "rule_types": [],
                    "always_bypass_integrations": [],
                    "bypass_identity_state": TOKEN_VAZIO,
                },
                "checks": {},
                "failures": [{"code": "NO_ACTIVE_DEFAULT_BRANCH_RULESET"}],
                "remediation": {
                    "authority": "GitHub provider ruleset administration",
                    "apply_state": "TOKEN_VAZIO_NOT_APPLIED_BY_THIS_EVALUATOR",
                    "operations": [{"kind": "ENSURE_ACTIVE_DEFAULT_BRANCH_RULESET"}],
                },
                "claim_allowed": False,
                "provider_apply_state": contract.get("provider_apply_state", TOKEN_VAZIO),
                "invariants": [
                    "TARGET_FILE != LIVE_PROVIDER_STATE",
                    "WORKFLOW_PASS != PROVIDER_ENFORCEMENT",
                    "TOKEN_VAZIO != PASS",
                ],
            }
        else:
            receipt = evaluate(
                contract,
                rulesets,
                target_path=str(args.target),
                target_sha256=file_sha256(args.target),
                repository=args.repository,
                default_branch=args.default_branch,
            )

    except (OSError, ValueError, json.JSONDecodeError, urllib.error.URLError) as exc:
        receipt = {
            "schema": "rafaelia.provider_protection_receipt/v2",
            "state": "ERROR",
            "gate": "FAIL",
            "repository": args.repository,
            "default_branch": args.default_branch,
            "failures": [{"code": "EVALUATOR_ERROR", "detail": str(exc)}],
            "claim_allowed": False,
            "invariants": [
                "OBSERVATION_ERROR != PASS",
                "TOKEN_VAZIO != PASS",
            ],
        }

    write_receipt(receipt, args.json, args.markdown)
    print(render_markdown(receipt), end="")
    print(f"receipt_json={args.json}")
    print(f"receipt_markdown={args.markdown}")
    return 0 if receipt.get("gate") == "PASS" else 4


if __name__ == "__main__":
    raise SystemExit(main())
