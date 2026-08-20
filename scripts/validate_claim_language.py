#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import re
from pathlib import Path
from typing import Any

RULES = {
    "fully_compatible": re.compile(r"\bfully compatible\b", re.I),
    "no_crash_guarantee": re.compile(r"\bwill\s+NOT\s+crash\b", re.I),
    "production_ready": re.compile(r"\bproduction[- ]ready\b", re.I),
    "guaranteed_compatible": re.compile(r"\bguaranteed(?:\s+\w+){0,2}\s+compatible\b", re.I),
    "zero_collisions": re.compile(r"\bZero Collisions\b", re.I),
}


def load_debt(path: Path) -> dict[str, Any]:
    value = json.loads(path.read_text(encoding="utf-8"))
    if not isinstance(value, dict) or value.get("schema") != "rafacodephi.claim-boundary-debt/v1":
        raise ValueError("invalid debt schema")
    return value


def validate(readme: Path, debt_path: Path) -> dict[str, Any]:
    errors: list[str] = []
    warnings: list[str] = []
    try:
        text = readme.read_text(encoding="utf-8")
        debt = load_debt(debt_path)
    except Exception as exc:
        return {"status": "FAIL", "errors": [str(exc)], "warnings": [], "claim_allowed": False}

    items = debt.get("items")
    if not isinstance(items, list):
        return {"status": "FAIL", "errors": ["debt.items must be array"], "warnings": [], "claim_allowed": False}

    allowances: dict[str, int] = {}
    for item in items:
        if not isinstance(item, dict):
            errors.append("debt item must be object")
            continue
        rule_id = item.get("rule_id")
        if rule_id not in RULES:
            errors.append(f"unknown debt rule_id {rule_id}")
            continue
        max_count = item.get("max_count")
        if not isinstance(max_count, int) or max_count < 0:
            errors.append(f"{rule_id}.max_count invalid")
            continue
        if item.get("claim_allowed") is not False:
            errors.append(f"{rule_id}.claim_allowed must be false")
        allowances[rule_id] = max_count

    findings: dict[str, Any] = {}
    for rule_id, pattern in RULES.items():
        matches = [match.group(0) for match in pattern.finditer(text)]
        findings[rule_id] = {"count": len(matches), "matches": matches}
        allowed = allowances.get(rule_id, 0)
        if len(matches) > allowed:
            errors.append(f"{rule_id}: found {len(matches)} > allowed known debt {allowed}")
        elif len(matches) < allowed:
            warnings.append(f"{rule_id}: known debt reduced {allowed}->{len(matches)}; update debt ledger after review")

    status = "FAIL" if errors else ("PASS_WITH_KNOWN_DEBT" if any(value["count"] for value in findings.values()) else "PASS")
    return {
        "schema_version": "rafacodephi.claim-language-validation/v1",
        "status": status,
        "claim_allowed": False,
        "errors": errors,
        "warnings": warnings,
        "findings": findings,
        "invariant": "new absolute compatibility/production claims cannot exceed explicitly bounded known debt"
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--readme", default="README.md")
    parser.add_argument("--debt", default="configs/claim-boundary-debt.v1.json")
    parser.add_argument("--write-report")
    args = parser.parse_args()
    report = validate(Path(args.readme), Path(args.debt))
    text = json.dumps(report, ensure_ascii=False, indent=2) + "\n"
    if args.write_report:
        output = Path(args.write_report)
        output.parent.mkdir(parents=True, exist_ok=True)
        output.write_text(text, encoding="utf-8")
    print(text, end="")
    return 1 if report["status"] == "FAIL" else 0


if __name__ == "__main__":
    raise SystemExit(main())
