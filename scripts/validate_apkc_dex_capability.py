#!/usr/bin/env python3
"""Validate APKC DEX capability claims without inference."""
from __future__ import annotations
import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CONFIG = ROOT / "configs/apkc-dex-capability.json"


def main() -> int:
    failures: list[str] = []
    checks: list[dict[str, object]] = []

    def check(name: str, condition: bool, detail: object) -> None:
        checks.append({"name": name, "status": "PASS" if condition else "FAIL", "detail": detail})
        if not condition:
            failures.append(name)

    try:
        data = json.loads(CONFIG.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        print(json.dumps({"schema": "raf.apkc-dex-capability-gate.v1", "status": "FAIL", "error": str(exc)}))
        return 1

    contracts = data.get("contracts", [])
    by_id = {item.get("id"): item for item in contracts if isinstance(item, dict)}
    check("schema", data.get("schema") == "raf.apkc-dex-capability.v1", data.get("schema"))
    check("automatic_promotion_disabled", data.get("automatic_claim_promotion") is False, data.get("automatic_claim_promotion"))
    check("unique_contract_ids", len(by_id) == len(contracts), list(by_id))
    check("release_blocked", data.get("release_allowed") is False, data.get("release_allowed"))

    for contract_id in ("empty-structural-dex035", "one-class-return-void-dex035"):
        item = by_id.get(contract_id, {})
        paths = [item.get("producer"), item.get("fixture"), item.get("validator")]
        check(contract_id + ".verified_claim", item.get("state") == "VERIFIED_HOST" and item.get("claim_allowed") is True, item)
        check(contract_id + ".evidence_present", all(isinstance(path, str) and path and (ROOT / path).is_file() for path in paths), paths)

    fixed = by_id.get("one-class-return-void-dex035", {}).get("fixed_layout", {})
    expected = {"file_size": 392, "strings": 4, "types": 3, "prototypes": 1, "methods": 1, "classes": 1, "code_units": 1, "map_items": 10}
    check("one_class_fixed_layout", fixed == expected, fixed)

    for contract_id in ("arbitrary-dex-backend", "multidex-merge", "art-dalvik-runtime", "java-kotlin-compiler"):
        item = by_id.get(contract_id, {})
        check(contract_id + ".not_promoted", item.get("state") == "TOKEN_VAZIO" and item.get("claim_allowed") is False and item.get("evidence") == [], item)

    source = (ROOT / "apkc/fmt_dex_one_class.h").read_text(encoding="utf-8")
    validator = (ROOT / "scripts/validate_apkc_one_class_dex.py").read_text(encoding="utf-8")
    check("source_scope_markers", all(token in source for token in ("Lraf/apkc/Stub;", "dex_build_one_class_checked", "DEX_ONE_OP_RETURN_VOID")), "one-class emitter")
    check("validator_scope_markers", all(token in validator for token in ("class_data_counts", "single_return_void", "map_entries_exact")), "semantic parser")

    report = {"schema": "raf.apkc-dex-capability-gate.v1", "status": "PASS" if not failures else "FAIL", "claim_allowed": not failures, "checks": checks, "failures": failures}
    json.dump(report, sys.stdout, ensure_ascii=False, indent=2)
    sys.stdout.write("\n")
    return 0 if not failures else 1


if __name__ == "__main__":
    raise SystemExit(main())
