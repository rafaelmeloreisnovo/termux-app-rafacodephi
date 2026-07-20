#!/usr/bin/env python3
"""Validate the APKC per-language capability matrix without inference.

Empty phase dictionaries inherit TOKEN_VAZIO from default_phase_record. A
language may be promoted to complete only when every required phase has an
accepted proof state and non-empty evidence.
"""

from __future__ import annotations

import argparse
import copy
import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
MATRIX_PATH = ROOT / "configs/compiler-capability-matrix.json"


def materialize_phase(language: dict[str, object], phase: str, default: dict[str, object]) -> dict[str, object]:
    phases = language.get("phases", {})
    if not isinstance(phases, dict):
        return copy.deepcopy(default)
    record = phases.get(phase)
    if not isinstance(record, dict):
        return copy.deepcopy(default)
    merged = copy.deepcopy(default)
    merged.update(record)
    return merged


def validate(matrix: dict[str, object]) -> dict[str, object]:
    failures: list[str] = []
    checks: list[dict[str, object]] = []

    def check(name: str, condition: bool, detail: object) -> None:
        checks.append({"name": name, "status": "PASS" if condition else "FAIL", "detail": detail})
        if not condition:
            failures.append(name)

    phase_order = matrix.get("phase_order", [])
    languages = matrix.get("languages", [])
    accepted = set(matrix.get("accepted_proof_states", []))
    default = matrix.get("default_phase_record", {})

    check("schema", matrix.get("schema") == "raf.apkc-compiler-capability-matrix.v1", matrix.get("schema"))
    check("automatic_promotion_disabled", matrix.get("automatic_claim_promotion") is False,
          matrix.get("automatic_claim_promotion"))
    check("phase_order_nonempty_unique", isinstance(phase_order, list) and bool(phase_order)
          and len(phase_order) == len(set(phase_order)), phase_order)
    check("languages_is_list", isinstance(languages, list), type(languages).__name__)
    check("default_is_token_vazio", isinstance(default, dict) and default.get("state") == "TOKEN_VAZIO"
          and default.get("evidence") == [], default)

    ids = [language.get("id") for language in languages if isinstance(language, dict)]
    check("unique_language_ids", len(ids) == len(set(ids)) and all(isinstance(item, str) and item for item in ids), ids)

    expanded: list[dict[str, object]] = []
    complete_count = 0

    for language in languages:
        if not isinstance(language, dict):
            failures.append("invalid_language_record")
            continue

        language_id = str(language.get("id", "TOKEN_VAZIO"))
        phase_report: dict[str, object] = {}
        proven_count = 0
        all_complete = True

        for phase in phase_order:
            record = materialize_phase(language, str(phase), default)
            state = record.get("state")
            evidence = record.get("evidence")
            proven = state in accepted and isinstance(evidence, list) and bool(evidence)
            if proven:
                proven_count += 1
            else:
                all_complete = False
            phase_report[str(phase)] = {
                "state": state,
                "evidence": evidence if isinstance(evidence, list) else [],
                "proven": proven,
            }

        declared_complete = language.get("complete_compiler") is True
        claim_allowed = language.get("claim_allowed") is True
        if declared_complete:
            complete_count += 1

        check(f"{language_id}.complete_requires_all_phases", not declared_complete or all_complete,
              {"declared_complete": declared_complete, "all_phases_proven": all_complete})
        check(f"{language_id}.claim_requires_complete", not claim_allowed or declared_complete,
              {"claim_allowed": claim_allowed, "declared_complete": declared_complete})
        check(f"{language_id}.no_false_positive", declared_complete == all_complete,
              {"declared_complete": declared_complete, "computed_complete": all_complete})

        expanded.append({
            "id": language_id,
            "display_name": language.get("display_name"),
            "candidate_extensions": language.get("candidate_extensions", []),
            "proven_phases": proven_count,
            "required_phases": len(phase_order),
            "completion_percent": round((100.0 * proven_count / len(phase_order)), 2) if phase_order else 0.0,
            "complete_compiler": all_complete,
            "claim_allowed": claim_allowed and all_complete,
            "phases": phase_report,
        })

    expected_count = matrix.get("current_result", {}).get("complete_apkc_owned_compilers") \
        if isinstance(matrix.get("current_result"), dict) else None
    check("declared_complete_count", expected_count == complete_count,
          {"declared": expected_count, "computed_from_declarations": complete_count})

    return {
        "schema": "raf.apkc-compiler-capability-report.v1",
        "status": "PASS" if not failures else "FAIL",
        "claim_allowed": not failures,
        "matrix": str(MATRIX_PATH.relative_to(ROOT)),
        "complete_apkc_owned_compilers": complete_count,
        "languages": expanded,
        "checks": checks,
        "failures": failures,
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--pretty", action="store_true")
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()

    try:
        matrix = json.loads(MATRIX_PATH.read_text(encoding="utf-8"))
        report = validate(matrix)
    except (OSError, json.JSONDecodeError) as exc:
        report = {
            "schema": "raf.apkc-compiler-capability-report.v1",
            "status": "FAIL",
            "claim_allowed": False,
            "error": str(exc),
        }

    rendered = json.dumps(report, ensure_ascii=False, indent=2 if args.pretty else None) + "\n"
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(rendered, encoding="utf-8")
    sys.stdout.write(rendered)
    return 0 if report.get("status") == "PASS" else 1


if __name__ == "__main__":
    raise SystemExit(main())
