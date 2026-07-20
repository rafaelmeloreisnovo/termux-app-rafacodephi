#!/usr/bin/env python3
"""Fail-closed validator for bounded repository claims."""
from __future__ import annotations
import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
MATRIX = ROOT / "configs/operational-technical-coherence.json"
DEX_CAP = ROOT / "configs/apkc-dex-capability.json"


def main() -> int:
    failures: list[str] = []
    checks: list[dict[str, object]] = []

    def check(name: str, ok: bool, detail: object) -> None:
        checks.append({"name": name, "status": "PASS" if ok else "FAIL", "detail": detail})
        if not ok:
            failures.append(name)

    try:
        matrix = json.loads(MATRIX.read_text(encoding="utf-8"))
        dex_cap = json.loads(DEX_CAP.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        print(json.dumps({"schema": "raf.operational-coherence-gate.v2", "status": "FAIL", "error": str(exc)}))
        return 1

    components = matrix.get("components", [])
    by_id = {item.get("id"): item for item in components if isinstance(item, dict)}
    states = set(matrix.get("states", []))
    ids = [item.get("id") for item in components if isinstance(item, dict)]

    check("unique_components", len(ids) == len(set(ids)) == len(components), ids)
    check("known_states", all(item.get("state") in states for item in components), [item.get("state") for item in components])
    check("claims_have_evidence", all(not item.get("claim_allowed") or bool(item.get("evidence")) for item in components),
          [item.get("id") for item in components if item.get("claim_allowed") and not item.get("evidence")])
    check("automatic_promotion_disabled", matrix.get("automatic_claim_promotion") is False, matrix.get("automatic_claim_promotion"))

    def component(component_id: str, state: str, claim: bool) -> dict[str, object]:
        item = by_id.get(component_id, {})
        check(component_id, item.get("state") == state and item.get("claim_allowed") is claim, item)
        return item

    browser = (ROOT / "Browser.sh").read_text(encoding="utf-8")
    check("legacy_https_downgrade_identified", "[FALLBACK] Usando HTTP para demo" in browser and "crypto não implementado" in browser, "Browser.sh")
    component("browser.https.failclosed.materializer", "VERIFIED_HOST", True)
    check("failclosed_tools", (ROOT / "scripts/materialize_browser_fail_closed.py").is_file() and (ROOT / "scripts/validate_browser_fail_closed.py").is_file(), "HTTPS tools")
    component("browser.tls13", "PROTOTYPE_FAIL_CLOSED_REQUIRED", False)
    component("browser.tls12", "DOCUMENT_ONLY", False)
    cert = component("browser.tls.certification", "TOKEN_VAZIO", False)
    check("tls_certification_no_evidence", cert.get("evidence") == [], cert)

    minimal = (ROOT / "apkc/fmt_dex.h").read_text(encoding="utf-8")
    one_class = (ROOT / "apkc/fmt_dex_one_class.h").read_text(encoding="utf-8")
    check("minimal_dex_markers", all(token in minimal for token in ("dex_build_checked", "no class definitions", "DEX_MAP_OFF")), "fmt_dex.h")
    component("apkc.dex.minimal", "VERIFIED_HOST", True)
    dex_exec = component("apkc.dex.executable-content", "VERIFIED_HOST", True)
    check("one_class_dex_scope", dex_exec.get("path") == "apkc/fmt_dex_one_class.h" and "fixed" in dex_exec.get("scope", "").lower(), dex_exec)
    check("one_class_dex_markers", all(token in one_class for token in ("dex_build_one_class_checked", "Lraf/apkc/Stub;", "DEX_ONE_OP_RETURN_VOID")), "fmt_dex_one_class.h")
    check("one_class_dex_tools", all((ROOT / path).is_file() for path in (
        "tests/native/apkc_emit_one_class_dex.c",
        "scripts/validate_apkc_one_class_dex.py",
        "scripts/validate_apkc_dex_capability.py",
    )), "DEX tools")
    dex_contracts = {item.get("id"): item for item in dex_cap.get("contracts", []) if isinstance(item, dict)}
    fixed = dex_contracts.get("one-class-return-void-dex035", {})
    check("dex_specific_matches_global", fixed.get("state") == "VERIFIED_HOST" and fixed.get("producer") == dex_exec.get("path"), fixed)
    for gap in ("arbitrary-dex-backend", "multidex-merge", "art-dalvik-runtime", "java-kotlin-compiler"):
        item = dex_contracts.get(gap, {})
        check("dex_gap_" + gap, item.get("state") == "TOKEN_VAZIO" and item.get("claim_allowed") is False and item.get("evidence") == [], item)

    elf = (ROOT / "apkc/fmt_elf.h").read_text(encoding="utf-8")
    check("elf_rel_markers", all(token in elf for token in ("apkc_elf32_arm_build_checked", "apkc_elf64_aarch64_build_checked", "APKC_ET_REL")), "fmt_elf.h")
    check("elf_exec_markers", all(token in elf for token in ("apkc_elf32_arm_exec_build_checked", "apkc_elf64_aarch64_exec_build_checked", "APKC_ET_EXEC", "APKC_PT_LOAD")), "fmt_elf.h")
    component("apkc.elf.custom-emitter", "VERIFIED_HOST", True)
    elf_exec = component("apkc.elf.executable-writer", "VERIFIED_HOST", True)
    check("elf_exec_is_fixed", "fixed" in elf_exec.get("scope", "").lower() and "not a general linker" in elf_exec.get("limitations", []), elf_exec)
    component("apkc.elf.linker", "TOKEN_VAZIO", False)
    component("apkc.elf.device-runtime", "TOKEN_VAZIO", False)
    check("elf_tools", all((ROOT / path).is_file() for path in (
        "tests/native/apkc_emit_minimal_elf.c", "tests/native/apkc_emit_exec_elf.c", "scripts/validate_apkc_elf_contract.py"
    )), "ELF tools")

    compiler = component("apkc.compiler.capability-matrix", "VERIFIED_HOST", True)
    check("compiler_matrix_is_governance", "governance" in " ".join(compiler.get("limitations", [])).lower(), compiler)
    check("compiler_tools", (ROOT / "configs/compiler-capability-matrix.json").is_file() and (ROOT / "scripts/validate_compiler_capability_matrix.py").is_file(), "compiler matrix")

    loose = component("documents.loose.corpus", "HISTORICAL_OR_LOOSE", False)
    check("loose_not_runtime", loose.get("claim_allowed") is False, loose)
    check("release_blocked", matrix.get("release_invariant", {}).get("current_result") is False, matrix.get("release_invariant"))

    report = {"schema": "raf.operational-coherence-gate.v2", "status": "PASS" if not failures else "FAIL", "claim_allowed": not failures, "checks": checks, "failures": failures}
    json.dump(report, sys.stdout, ensure_ascii=False, indent=2)
    sys.stdout.write("\n")
    return 0 if not failures else 1


if __name__ == "__main__":
    raise SystemExit(main())
