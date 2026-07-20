#!/usr/bin/env python3
"""Validate and render the first-part evidence-weighted gap map."""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
MAP_PATH = ROOT / "configs/first-part-gap-map.json"


def validate(data: dict[str, object]) -> dict[str, object]:
    failures: list[str] = []
    checks: list[dict[str, object]] = []

    def check(name: str, condition: bool, detail: object) -> None:
        checks.append({"name": name, "status": "PASS" if condition else "FAIL", "detail": detail})
        if not condition:
            failures.append(name)

    weights = data.get("criterion_states", {})
    domains = data.get("domains", [])
    check("schema", data.get("schema") in {"raf.first-part-gap-map.v1", "raf.first-part-gap-map.v2"}, data.get("schema"))
    check("automatic_promotion_disabled", data.get("automatic_claim_promotion") is False,
          data.get("automatic_claim_promotion"))
    check("release_blocked", data.get("release_allowed") is False, data.get("release_allowed"))
    check("weights", isinstance(weights, dict) and weights.get("PROVEN") == 1.0
          and weights.get("TOKEN_VAZIO") == 0.0, weights)
    check("domains", isinstance(domains, list) and bool(domains), type(domains).__name__)

    ids = [item.get("id") for item in domains if isinstance(item, dict)]
    check("unique_domain_ids", len(ids) == len(set(ids)) and all(isinstance(item, str) for item in ids), ids)

    rendered_domains: list[dict[str, object]] = []
    total_points = 0.0
    total_criteria = 0

    for domain in domains:
        if not isinstance(domain, dict):
            failures.append("invalid_domain_record")
            continue
        domain_id = str(domain.get("id", "TOKEN_VAZIO"))
        criteria = domain.get("criteria", [])
        if not isinstance(criteria, list) or not criteria:
            failures.append(f"{domain_id}.criteria")
            continue

        criterion_ids = [item.get("id") for item in criteria if isinstance(item, dict)]
        check(f"{domain_id}.unique_criteria", len(criterion_ids) == len(set(criterion_ids)), criterion_ids)

        points = 0.0
        state_counts: dict[str, int] = {}
        for criterion in criteria:
            if not isinstance(criterion, dict):
                failures.append(f"{domain_id}.invalid_criterion")
                continue
            state = criterion.get("state")
            evidence = criterion.get("evidence")
            if state not in weights:
                failures.append(f"{domain_id}.{criterion.get('id')}.unknown_state")
                continue
            weight = float(weights[state])
            state_counts[str(state)] = state_counts.get(str(state), 0) + 1
            points += weight
            if weight > 0.0 and (not isinstance(evidence, list) or not evidence):
                failures.append(f"{domain_id}.{criterion.get('id')}.missing_evidence")

        count = len(criteria)
        percent = round(100.0 * points / count, 2)
        total_points += points
        total_criteria += count
        rendered_domains.append({
            "id": domain_id,
            "evidence_coverage_percent": percent,
            "earned_points": points,
            "possible_points": count,
            "state_counts": state_counts,
            "claim_allowed": domain.get("claim_allowed") is True,
            "claim_scope": domain.get("claim_scope"),
        })

    overall = round(100.0 * total_points / total_criteria, 2) if total_criteria else 0.0
    return {
        "schema": "raf.first-part-gap-report.v2",
        "source_schema": data.get("schema"),
        "status": "PASS" if not failures else "FAIL",
        "measurement": data.get("measurement"),
        "warning": data.get("warning"),
        "overall_evidence_coverage_percent": overall,
        "release_allowed": False,
        "domains": rendered_domains,
        "checks": checks,
        "failures": failures,
    }


def markdown(report: dict[str, object]) -> str:
    rows = [
        "# First-part evidence coverage",
        "",
        "> This percentage measures registered evidence criteria. It is not certification or product readiness.",
        "",
        "| Domain | Evidence coverage | Claim scope |",
        "|---|---:|---|",
    ]
    for domain in report.get("domains", []):
        rows.append(
            f"| `{domain['id']}` | {domain['evidence_coverage_percent']:.2f}% | {domain.get('claim_scope') or 'TOKEN_VAZIO'} |"
        )
    rows.extend([
        "",
        f"**Overall evidence coverage:** {report.get('overall_evidence_coverage_percent', 0):.2f}%",
        "",
        "**Release allowed:** `false`",
        "",
    ])
    return "\n".join(rows)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--pretty", action="store_true")
    parser.add_argument("--format", choices=("json", "markdown"), default="json")
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()

    try:
        data = json.loads(MAP_PATH.read_text(encoding="utf-8"))
        report = validate(data)
    except (OSError, json.JSONDecodeError) as exc:
        report = {"schema": "raf.first-part-gap-report.v2", "status": "FAIL", "error": str(exc)}

    if args.format == "markdown":
        rendered = markdown(report)
    else:
        rendered = json.dumps(report, ensure_ascii=False, indent=2 if args.pretty else None)
    rendered += "\n"

    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(rendered, encoding="utf-8")
    sys.stdout.write(rendered)
    return 0 if report.get("status") == "PASS" else 1


if __name__ == "__main__":
    raise SystemExit(main())
