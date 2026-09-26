#!/usr/bin/env python3
"""Evaluate the live GitHub default-branch ruleset against the canonical target.

SOURCE != PROVIDER_STATE != OBSERVATION != EVIDENCE != CLAIM.

The desired state lives in governance/provider/PROVIDER_RULESET_TARGET.v3.json.
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
from datetime import datetime, timezone
from pathlib import Path
from typing import Any

DEFAULT_TARGET = Path("governance/provider/PROVIDER_RULESET_TARGET.v3.json")
DEFAULT_WITNESS = Path("governance/provider/PROVIDER_RULESET_EXTERNAL_WITNESS_20260926.v2.json")
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
    if data.get("schema") not in {"rafaelia.provider_ruleset_target/v1", "rafaelia.provider_ruleset_target/v2", "rafaelia.provider_ruleset_target/v3"}:
        raise ValueError(f"unsupported target schema: {data.get('schema')!r}")
    if not isinstance(data.get("target"), dict):
        raise ValueError("target contract missing object: target")
    return data


def load_witness(path: Path) -> dict[str, Any]:
    data = json.loads(path.read_text(encoding="utf-8"))
    if data.get("schema") not in {"rafaelia.provider_ruleset_external_witness/v1", "rafaelia.provider_ruleset_external_witness/v2"}:
        raise ValueError(f"unsupported witness schema: {data.get('schema')!r}")
    if not isinstance(data.get("binding"), dict):
        raise ValueError("witness missing binding object")
    if not isinstance(data.get("observed"), dict):
        raise ValueError("witness missing observed object")
    return data


def github_get(url: str, token: str) -> Any:
    headers = {
        "Accept": "application/vnd.github+json",
        "User-Agent": "rafaelia-provider-protection-contract-v3",
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


def _normalize_bypass_actor(actor: Any) -> dict[str, Any] | None:
    if not isinstance(actor, dict):
        return None
    actor_type = actor.get("actor_type")
    actor_id = actor.get("actor_id")
    bypass_mode = actor.get("bypass_mode")
    if not isinstance(actor_type, str) or not isinstance(actor_id, int):
        return None
    if bypass_mode != "always":
        return None
    return {
        "actor_type": actor_type,
        "actor_id": actor_id,
        "bypass_mode": "always",
    }


def _actor_key(actor: dict[str, Any]) -> tuple[str, int]:
    return str(actor["actor_type"]), int(actor["actor_id"])


def _always_bypass_actors(rulesets: list[dict[str, Any]]) -> list[dict[str, Any]]:
    actors: dict[tuple[str, int], dict[str, Any]] = {}
    for rs in rulesets:
        for raw in rs.get("bypass_actors") or []:
            actor = _normalize_bypass_actor(raw)
            if actor is not None:
                actors[_actor_key(actor)] = actor
    return [actors[key] for key in sorted(actors)]


def _integration_ids(actors: list[dict[str, Any]]) -> list[int]:
    return sorted(
        actor["actor_id"]
        for actor in actors
        if actor.get("actor_type") == "Integration"
    )


def _witness_bypass_actors(witness: dict[str, Any]) -> list[dict[str, Any]]:
    observed = witness.get("observed") or {}
    raw_actors = observed.get("always_bypass_actors")
    if isinstance(raw_actors, list):
        actors = [
            actor
            for raw in raw_actors
            if (actor := _normalize_bypass_actor(raw)) is not None
        ]
        return sorted(actors, key=_actor_key)

    # V1 compatibility: Integration IDs were the only modeled actor class.
    return [
        {
            "actor_type": "Integration",
            "actor_id": actor_id,
            "bypass_mode": "always",
        }
        for actor_id in sorted(
            item
            for item in (observed.get("always_bypass_integrations") or [])
            if isinstance(item, int)
        )
    ]


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


def _same_instant(left: Any, right: Any) -> bool:
    if not isinstance(left, str) or not isinstance(right, str):
        return left == right
    try:
        def parse(value: str) -> datetime:
            normalized = value[:-1] + "+00:00" if value.endswith("Z") else value
            return datetime.fromisoformat(normalized).astimezone(timezone.utc)
        return parse(left) == parse(right)
    except ValueError:
        return left == right


def _resolve_bypass_observation(
    rulesets: list[dict[str, Any]],
    witness: dict[str, Any] | None,
) -> tuple[list[dict[str, Any]], str, dict[str, Any], Any]:
    direct_actors = _always_bypass_actors(rulesets)
    direct_user_values = sorted(
        {
            value
            for rs in rulesets
            if isinstance((value := rs.get("current_user_can_bypass")), str)
        }
    )
    direct_user_bypass: Any = (
        TOKEN_VAZIO
        if not direct_user_values
        else (
            direct_user_values[0]
            if len(direct_user_values) == 1
            else direct_user_values
        )
    )

    if witness is None:
        return direct_actors, (
            "DIRECT_LIVE_OBSERVATION"
            if direct_actors
            else "LIVE_EMPTY_NO_EXTERNAL_WITNESS"
        ), {
            "witness_used": False,
            "witness_match": False,
        }, direct_user_bypass

    binding = witness.get("binding") or {}
    witness_id = binding.get("ruleset_id")
    witness_updated_at = binding.get("ruleset_updated_at")
    matched = any(
        rs.get("id") == witness_id
        and _same_instant(rs.get("updated_at"), witness_updated_at)
        for rs in rulesets
    )
    if not matched:
        return direct_actors, "TOKEN_VAZIO_STALE_OR_UNMATCHED_WITNESS", {
            "witness_used": True,
            "witness_match": False,
            "binding_ruleset_id": witness_id,
            "binding_ruleset_updated_at": witness_updated_at,
            "observed_ruleset_versions": [
                {"id": rs.get("id"), "updated_at": rs.get("updated_at")}
                for rs in rulesets
            ],
        }, direct_user_bypass

    merged = {_actor_key(actor): actor for actor in direct_actors}
    for actor in _witness_bypass_actors(witness):
        merged[_actor_key(actor)] = actor

    witness_user_bypass = (witness.get("observed") or {}).get(
        "current_user_can_bypass",
        TOKEN_VAZIO,
    )
    current_user = (
        witness_user_bypass
        if witness_user_bypass != TOKEN_VAZIO
        else (direct_user_bypass)
    )
    return [merged[key] for key in sorted(merged)], "BOUND_EXTERNAL_WITNESS", {
        "witness_used": True,
        "witness_match": True,
        "binding_ruleset_id": witness_id,
        "binding_ruleset_updated_at": witness_updated_at,
        "direct_actor_count": len(direct_actors),
        "witness_actor_count": len(_witness_bypass_actors(witness)),
    }, current_user


def _check_bypass_policy(
    target: dict[str, Any],
    rulesets: list[dict[str, Any]],
    witness: dict[str, Any] | None = None,
) -> tuple[bool, dict[str, Any]]:
    policy = target.get("bypass_policy") or {}
    observed_actors, assurance, witness_state, current_user_bypass = (
        _resolve_bypass_observation(rulesets, witness)
    )

    if "justified_actors" in policy:
        justified_actors = [
            actor
            for raw in (policy.get("justified_actors") or [])
            if (actor := _normalize_bypass_actor({
                **raw,
                "bypass_mode": raw.get("bypass_mode", "always"),
            } if isinstance(raw, dict) else raw)) is not None
        ]
    else:
        justified_actors = [
            {
                "actor_type": "Integration",
                "actor_id": actor_id,
                "bypass_mode": "always",
            }
            for actor_id in (policy.get("justified_integration_ids") or [])
            if isinstance(actor_id, int)
        ]

    justified_keys = {_actor_key(actor) for actor in justified_actors}
    unresolved_actors = [
        actor
        for actor in observed_actors
        if _actor_key(actor) not in justified_keys
    ]
    visibility_unproven = assurance == "TOKEN_VAZIO_STALE_OR_UNMATCHED_WITNESS"

    return not unresolved_actors and not visibility_unproven, {
        "observed_always_bypass_actors": observed_actors,
        "observed_always_bypass_integration_ids": _integration_ids(observed_actors),
        "justified_actors": sorted(justified_actors, key=_actor_key),
        "justified_integration_ids": _integration_ids(justified_actors),
        "unresolved_actors": unresolved_actors,
        "unresolved_integration_ids": _integration_ids(unresolved_actors),
        "current_user_can_bypass": current_user_bypass,
        "observation_assurance": assurance,
        "visibility_unproven": visibility_unproven,
        "witness_state": witness_state,
        "policy": policy.get(
            "always_bypass_actors",
            policy.get(
                "always_bypass_integrations",
                "TOKEN_VAZIO_NO_EXPLICIT_BYPASS_POLICY",
            ),
        ),
    }


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
    witness: dict[str, Any] | None = None,
) -> dict[str, Any]:
    target = contract["target"]
    by_type = _rules_by_type(rulesets)

    if "required_rules" in target:
        required_types = sorted(set(target.get("required_rules") or []))
    else:
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
    bypass_ok, bypass_check = _check_bypass_policy(target, rulesets, witness)

    bypass_actors = bypass_check["observed_always_bypass_actors"]
    bypass_ids = bypass_check["observed_always_bypass_integration_ids"]
    bypass_state = (
        "TOKEN_VAZIO_STALE_OR_UNMATCHED_WITNESS"
        if bypass_check["visibility_unproven"]
        else (
            "TOKEN_VAZIO_PENDING_IDENTITY_AND_JUSTIFICATION"
            if bypass_check["unresolved_actors"]
            else "NONE_UNRESOLVED"
        )
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
    if bypass_check["visibility_unproven"]:
        failures.append(
            {
                "code": "BYPASS_VISIBILITY_UNPROVEN",
                "observation_assurance": bypass_check["observation_assurance"],
                "witness_state": bypass_check["witness_state"],
            }
        )
    elif not bypass_ok:
        failures.append(
            {
                "code": "UNJUSTIFIED_ALWAYS_BYPASS_ACTORS",
                "observed": bypass_check["observed_always_bypass_actors"],
                "justified": bypass_check["justified_actors"],
                "unresolved": bypass_check["unresolved_actors"],
                "observation_assurance": bypass_check["observation_assurance"],
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
    if bypass_check["visibility_unproven"]:
        remediation["operations"].append(
            {
                "kind": "REFRESH_BYPASS_WITNESS_OR_PROVIDE_PRIVILEGED_OBSERVATION",
                "witness_state": bypass_check["witness_state"],
            }
        )
    elif not bypass_ok:
        remediation["operations"].append(
            {
                "kind": "IDENTIFY_JUSTIFY_OR_REMOVE_ALWAYS_BYPASS_ACTORS",
                "actors": bypass_check["unresolved_actors"],
            }
        )

    return {
        "schema": "rafaelia.provider_protection_receipt/v3",
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
            "always_bypass_actors": bypass_actors,
            "always_bypass_integrations": bypass_ids,
            "current_user_can_bypass": bypass_check["current_user_can_bypass"],
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
            "always_bypass_actors": {
                "pass": bypass_ok,
                **bypass_check,
            },
        },
        "failures": failures,
        "remediation": remediation,
        "claim_allowed": False,
        "provider_apply_state": TOKEN_VAZIO,
        "invariants": [
            "TARGET_FILE != LIVE_PROVIDER_STATE",
            "WITNESS != LIVE_PROVIDER_STATE",
            "WITNESS_VALID_ONLY_WHILE_RULESET_VERSION_MATCHES",
            "WORKFLOW_PASS != PROVIDER_ENFORCEMENT",
            "TOKEN_VAZIO != PASS",
        ],
    }


def render_markdown(receipt: dict[str, Any]) -> str:
    checks = receipt.get("checks") or {}

    def check_state(key: str) -> str:
        return "PASS" if (checks.get(key) or {}).get("pass") is True else "FAIL"

    lines = [
        "# Provider Protection Receipt V3",
        "",
        f"- state: **{receipt.get('state', TOKEN_VAZIO)}**",
        f"- gate: **{receipt.get('gate', 'FAIL')}**",
        f"- repository: `{receipt.get('repository', TOKEN_VAZIO)}`",
        f"- default branch: `{receipt.get('default_branch', TOKEN_VAZIO)}`",
    ]

    target = receipt.get("target") or {}
    live = receipt.get("live_observation") or {}
    if target:
        lines.append(f"- target SHA-256: `{target.get('sha256', TOKEN_VAZIO)}`")
    if live:
        lines.extend(
            [
                f"- live digest SHA-256: `{live.get('digest_sha256', TOKEN_VAZIO)}`",
                f"- live ruleset IDs: `{live.get('ruleset_ids', [])}`",
                f"- bypass identity state: **{live.get('bypass_identity_state', TOKEN_VAZIO)}**",
            ]
        )

    if checks:
        lines.extend(
            [
                "",
                "## Gates",
                "",
                "| Gate | State |",
                "|---|---|",
                f"| required rule types | {check_state('required_rule_types')} |",
                f"| pull request policy | {check_state('pull_request')} |",
                f"| required status checks | {check_state('required_status_checks')} |",
                f"| always-bypass actors | {check_state('always_bypass_actors')} |",
            ]
        )

    failures = receipt.get("failures") or []
    if failures:
        lines.extend(["", "## Failures", ""])
        for failure in failures:
            lines.append(f"- **{failure.get('code', 'UNKNOWN_FAILURE')}**")

    remediation = receipt.get("remediation") or {}
    lines.extend(
        [
            "",
            "## Boundary",
            "",
            "- `TARGET_FILE != LIVE_PROVIDER_STATE`",
            "- `WITNESS != LIVE_PROVIDER_STATE`",
            "- `WORKFLOW_PASS != PROVIDER_ENFORCEMENT`",
            "- `claim_allowed=false`",
            f"- provider apply state: **{remediation.get('apply_state', TOKEN_VAZIO)}**",
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
    parser.add_argument("--witness", type=Path, default=DEFAULT_WITNESS)
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
        witness = load_witness(args.witness)
        if witness.get("repository") != args.repository:
            raise ValueError(
                f"witness repository mismatch: {witness.get('repository')} != {args.repository}"
            )
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
                "schema": "rafaelia.provider_protection_receipt/v3",
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
                    "always_bypass_actors": [],
                    "always_bypass_integrations": [],
                    "current_user_can_bypass": TOKEN_VAZIO,
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
                witness=witness,
            )
            receipt["witness"] = {
                "path": str(args.witness),
                "sha256": file_sha256(args.witness),
                "schema": witness["schema"],
                "binding": witness.get("binding"),
            }

    except (OSError, ValueError, json.JSONDecodeError, urllib.error.URLError) as exc:
        receipt = {
            "schema": "rafaelia.provider_protection_receipt/v3",
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

    observed_at = datetime.now(timezone.utc).replace(microsecond=0).isoformat().replace("+00:00", "Z")
    run_id = os.environ.get("GITHUB_RUN_ID")
    run_attempt = os.environ.get("GITHUB_RUN_ATTEMPT")
    observation_id = (
        f"github-actions:{args.repository}:{run_id}:{run_attempt or '1'}"
        if run_id
        else f"local:{args.repository}:{observed_at}"
    )
    receipt.setdefault("observed_at_utc", observed_at)
    receipt.setdefault("provider_observation_id", observation_id)
    receipt.setdefault(
        "execution",
        {
            "github_sha": os.environ.get("GITHUB_SHA", TOKEN_VAZIO),
            "github_run_id": run_id or TOKEN_VAZIO,
            "github_run_attempt": run_attempt or TOKEN_VAZIO,
            "github_event_name": os.environ.get("GITHUB_EVENT_NAME", TOKEN_VAZIO),
        },
    )

    write_receipt(receipt, args.json, args.markdown)
    print(render_markdown(receipt), end="")
    print(f"receipt_json={args.json}")
    print(f"receipt_markdown={args.markdown}")
    return 0 if receipt.get("gate") == "PASS" else 4


if __name__ == "__main__":
    raise SystemExit(main())
