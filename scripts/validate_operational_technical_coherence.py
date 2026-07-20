#!/usr/bin/env python3
"""Fail-closed operational coherence validator.

This gate does not pretend to prove device runtime. It verifies that repository
claims remain bounded by the source and evidence currently present.
"""

from __future__ import annotations

import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
MATRIX_PATH = ROOT / "configs/operational-technical-coherence.json"


def main() -> int:
    failures: list[str] = []
    checks: list[dict[str, object]] = []

    def check(name: str, condition: bool, detail: object) -> None:
        checks.append({"name": name, "status": "PASS" if condition else "FAIL", "detail": detail})
        if not condition:
            failures.append(name)

    try:
        matrix = json.loads(MATRIX_PATH.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        print(json.dumps({"schema": "raf.operational-coherence-gate.v1", "status": "FAIL", "error": str(exc)}))
        return 1

    components = matrix.get("components", [])
    states = set(matrix.get("states", []))
    by_id = {component.get("id"): component for component in components}

    ids = [component.get("id") for component in components]
    check("unique_component_ids", len(ids) == len(set(ids)) and all(isinstance(item, str) for item in ids), ids)
    check("known_states", all(component.get("state") in states for component in components),
          [component.get("state") for component in components])
    check(
        "claims_require_evidence",
        all(not component.get("claim_allowed") or bool(component.get("evidence")) for component in components),
        [component.get("id") for component in components if component.get("claim_allowed") and not component.get("evidence")],
    )
    check("no_automatic_claim_promotion", matrix.get("automatic_claim_promotion") is False,
          matrix.get("automatic_claim_promotion"))

    browser_path = ROOT / "Browser.sh"
    browser = browser_path.read_text(encoding="utf-8") if browser_path.is_file() else ""
    tls13 = by_id.get("browser.tls13", {})
    tls12 = by_id.get("browser.tls12", {})
    tls_cert = by_id.get("browser.tls.certification", {})
    failclosed = by_id.get("browser.https.failclosed.materializer", {})

    legacy_tls_markers = (
        "crypto não implementado" in browser
        and "X25519 + AES-GCM + HKDF" in browser
        and "[FALLBACK] Usando HTTP para demo" in browser
    )
    check(
        "browser_legacy_tls_downgrade_identified",
        legacy_tls_markers,
        "raw Browser.sh remains a historical prototype and must not be the canonical build entrypoint",
    )
    check(
        "browser_failclosed_materializer_registered",
        failclosed.get("state") == "VERIFIED_HOST"
        and failclosed.get("claim_allowed") is True
        and failclosed.get("path") == "scripts/materialize_browser_fail_closed.py"
        and bool(failclosed.get("evidence")),
        failclosed,
    )
    check(
        "browser_failclosed_tools_present",
        (ROOT / "scripts/materialize_browser_fail_closed.py").is_file()
        and (ROOT / "scripts/validate_browser_fail_closed.py").is_file(),
        ["scripts/materialize_browser_fail_closed.py", "scripts/validate_browser_fail_closed.py"],
    )
    check(
        "browser_tls13_not_promoted",
        tls13.get("state") == "PROTOTYPE_FAIL_CLOSED_REQUIRED" and tls13.get("claim_allowed") is False,
        tls13,
    )
    check(
        "browser_tls12_not_promoted",
        tls12.get("state") == "DOCUMENT_ONLY" and tls12.get("claim_allowed") is False,
        tls12,
    )
    check(
        "tls_certification_requires_external_evidence",
        tls_cert.get("state") == "TOKEN_VAZIO" and tls_cert.get("claim_allowed") is False and not tls_cert.get("evidence"),
        tls_cert,
    )

    dex_path = ROOT / "apkc/fmt_dex.h"
    dex = dex_path.read_text(encoding="utf-8") if dex_path.is_file() else ""
    dex_tokens = (
        "const u64 message_bits = c->bits;",
        "w32(out + 104, DEX_DATA_SZ);",
        "w32(out + 108, DEX_MAP_OFF);",
        "dex_build_checked",
        "no class definitions",
    )
    check("dex_contract_source_markers", all(token in dex for token in dex_tokens), dex_tokens)
    check(
        "dex_scope_is_structural",
        by_id.get("apkc.dex.minimal", {}).get("scope", "").startswith("DEX 035 structural"),
        by_id.get("apkc.dex.minimal"),
    )
    dex_exec = by_id.get("apkc.dex.executable-content", {})
    check(
        "dex_executable_content_not_inferred",
        dex_exec.get("state") == "TOKEN_VAZIO"
        and dex_exec.get("claim_allowed") is False
        and not dex_exec.get("evidence"),
        dex_exec,
    )

    sys_path = ROOT / "apkc/sys.h"
    sys_text = sys_path.read_text(encoding="utf-8") if sys_path.is_file() else ""
    check(
        "apkc_host_test_is_explicit",
        "RAF_APKC_HOST_TEST" in sys_text and "supports only ARM32/ARM64" in sys_text,
        "host adapter and unsupported-architecture fail-fast",
    )

    elf_text = (ROOT / "apkc/fmt_elf.h").read_text(encoding="utf-8") \
        if (ROOT / "apkc/fmt_elf.h").is_file() else ""
    elf_structural = by_id.get("apkc.elf.custom-emitter", {})
    elf_executable = by_id.get("apkc.elf.executable-writer", {})
    elf_tokens = (
        "apkc_elf32_arm_build_checked",
        "apkc_elf64_aarch64_build_checked",
        "APKC_ET_REL",
        "null section only",
    )
    check("elf_structural_source_markers", all(token in elf_text for token in elf_tokens), elf_tokens)
    check(
        "elf_structural_emitter_bounded_claim",
        elf_structural.get("state") == "VERIFIED_HOST"
        and elf_structural.get("claim_allowed") is True
        and bool(elf_structural.get("evidence")),
        elf_structural,
    )
    check(
        "elf_structural_tools_present",
        (ROOT / "tests/native/apkc_emit_minimal_elf.c").is_file()
        and (ROOT / "scripts/validate_apkc_elf_contract.py").is_file(),
        ["tests/native/apkc_emit_minimal_elf.c", "scripts/validate_apkc_elf_contract.py"],
    )
    check(
        "elf_executable_not_inferred",
        elf_executable.get("state") == "TOKEN_VAZIO"
        and elf_executable.get("claim_allowed") is False
        and not elf_executable.get("evidence"),
        elf_executable,
    )

    compiler_matrix = by_id.get("apkc.compiler.capability-matrix", {})
    check(
        "compiler_matrix_is_governance_only",
        compiler_matrix.get("state") == "VERIFIED_HOST"
        and compiler_matrix.get("claim_allowed") is True
        and "governance" in " ".join(compiler_matrix.get("limitations", [])).lower(),
        compiler_matrix,
    )
    check(
        "compiler_matrix_tools_present",
        (ROOT / "configs/compiler-capability-matrix.json").is_file()
        and (ROOT / "scripts/validate_compiler_capability_matrix.py").is_file(),
        ["configs/compiler-capability-matrix.json", "scripts/validate_compiler_capability_matrix.py"],
    )

    gap_map = by_id.get("governance.first-part-gap-map", {})
    check(
        "first_part_gap_map_registered",
        gap_map.get("state") == "VERIFIED_HOST"
        and gap_map.get("claim_allowed") is True
        and bool(gap_map.get("evidence")),
        gap_map,
    )
    check(
        "first_part_gap_map_tools_present",
        (ROOT / "configs/first-part-gap-map.json").is_file()
        and (ROOT / "scripts/validate_first_part_gap_map.py").is_file(),
        ["configs/first-part-gap-map.json", "scripts/validate_first_part_gap_map.py"],
    )

    loose = by_id.get("documents.loose.corpus", {})
    check(
        "loose_documents_are_not_runtime",
        loose.get("state") == "HISTORICAL_OR_LOOSE" and loose.get("claim_allowed") is False,
        loose,
    )

    release = matrix.get("release_invariant", {})
    check("release_remains_blocked", release.get("current_result") is False, release)

    report = {
        "schema": "raf.operational-coherence-gate.v1",
        "status": "PASS" if not failures else "FAIL",
        "claim_allowed": not failures,
        "matrix": str(MATRIX_PATH.relative_to(ROOT)),
        "checks": checks,
        "failures": failures,
    }
    json.dump(report, sys.stdout, ensure_ascii=False, indent=2)
    sys.stdout.write("\n")
    return 0 if not failures else 1


if __name__ == "__main__":
    raise SystemExit(main())
