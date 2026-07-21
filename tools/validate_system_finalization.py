#!/usr/bin/env python3
"""Evaluate RAFCODEPHI completion without conflating static closure and release.

The validator has three profiles:

* safe-core: source, contracts and fail-closed gates are coherent;
* functional-distribution: real package stack, signing, CI and device evidence;
* full-platform: distribution plus research components promoted to complete systems.

A PASS for safe-core never promotes release_allowed.
"""
from __future__ import annotations

import argparse
import importlib.util
import json
import subprocess
import sys
from pathlib import Path
from typing import Any

ROOT = Path(__file__).resolve().parents[1]
CONTRACT_PATH = ROOT / "configs" / "system-finalization-contract.json"


def load_json(path: Path) -> dict[str, Any]:
    return json.loads(path.read_text(encoding="utf-8"))


def parse_properties(path: Path) -> dict[str, str]:
    values: dict[str, str] = {}
    for raw in path.read_text(encoding="utf-8").splitlines():
        line = raw.strip()
        if not line or line.startswith("#") or "=" not in line:
            continue
        key, value = line.split("=", 1)
        values[key.strip()] = value.strip()
    return values


def result(ok: bool, state: str, detail: str, evidence: list[str] | None = None) -> dict[str, Any]:
    return {
        "ok": bool(ok),
        "state": state,
        "detail": detail,
        "evidence": evidence or [],
    }


def check_build_metadata(root: Path) -> dict[str, Any]:
    path = root / "gradle.properties"
    if not path.exists():
        return result(False, "MISSING", "gradle.properties is absent")
    props = parse_properties(path)
    expected = {
        "minSdkVersion": "21",
        "targetSdkVersion": "28",
        "compileSdkVersion": "35",
        "ndkVersion": "26.3.11579264",
        "termux.abi.matrix": "armeabi-v7a,arm64-v8a",
        "termux.abi.optional": "",
        "termux.abi.universal": "true",
    }
    mismatches = {
        key: {"expected": value, "actual": props.get(key)}
        for key, value in expected.items()
        if props.get(key) != value
    }
    if mismatches:
        return result(False, "DIVERGENT", json.dumps(mismatches, sort_keys=True), [str(path.relative_to(root))])
    return result(True, "PROVEN_STRUCTURAL", "canonical Android/NDK/ABI metadata matches", [str(path.relative_to(root))])


def check_action_references(root: Path) -> dict[str, Any]:
    script = root / "scripts" / "audit_github_actions_refs.py"
    if not script.exists():
        return result(False, "MISSING", "GitHub Actions reference auditor is absent")
    spec = importlib.util.spec_from_file_location("raf_action_audit", script)
    if spec is None or spec.loader is None:
        return result(False, "LOAD_ERROR", "cannot load action reference auditor")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    records = module.audit(root)
    failures = [record for record in records if record["state"] in module.FAIL_STATES]
    if failures:
        compact = [f"{item['file']}:{item['line']} {item['action']}@{item['ref']}" for item in failures]
        return result(False, "POLICY_VIOLATION", "; ".join(compact), [str(script.relative_to(root))])
    return result(True, "PROVEN_STRUCTURAL", f"{len(records)} action references classified; no strict violation", [str(script.relative_to(root))])


def check_loader_quarantine(root: Path) -> dict[str, Any]:
    contract_path = root / "configs" / "loader-functional-security-contract.json"
    report_path = root / "reports" / "loader-functional-quarantine-local-validation.json"
    if not contract_path.exists() or not report_path.exists():
        return result(False, "MISSING", "loader contract or quarantine evidence is absent")
    contract = load_json(contract_path)
    report = load_json(report_path)
    allowed = set(contract.get("allowed_states", []))
    state = report.get("state")
    checks_total = report.get("checks_total")
    checks_passed = report.get("checks_passed")
    ok = (
        report.get("status") == "PASS"
        and state in allowed
        and checks_total == checks_passed
        and checks_total not in (None, 0)
        and report.get("release_allowed") is False
        and report.get("claim_allowed") is False
        and contract.get("release_allowed") is False
    )
    if not ok:
        return result(False, "UNSAFE_OR_UNPROVEN", f"loader state={state!r}, checks={checks_passed}/{checks_total}", [str(contract_path.relative_to(root)), str(report_path.relative_to(root))])
    return result(True, state, f"loader is accepted only in quarantined state {state}; release remains blocked", [str(contract_path.relative_to(root)), str(report_path.relative_to(root))])


def check_zero_instrumentation(root: Path) -> dict[str, Any]:
    path = root / "reports" / "RAFAELIA_ZERO_OPERATIONAL_EVIDENCE_BASELINE_20260720.json"
    if not path.exists():
        return result(False, "MISSING", "RAFAELIA ZERO operational evidence baseline is absent")
    data = load_json(path)
    implemented = data.get("implemented", {})
    required = {
        "debug_probe",
        "app_private_atomic_receipt",
        "installed_apk_capture_via_adb_pull",
        "single_installed_apk_path_gate",
        "input_installed_apk_hash_match",
        "receipt_sha256",
        "apk_sha256",
        "transcript_hash_binding",
        "atomic_bundle_publication",
        "atomic_selection_pointer",
        "atomic_matrix_publication",
        "one_validated_bundle_per_role",
        "selection_path_traversal_rejection",
        "selection_symlink_rejection",
        "bundle_symlink_rejection",
        "manifest_digest_validation",
        "sha256sums_validation",
        "anti_replay_matrix",
        "dual_architecture_promotion",
    }
    missing = sorted(key for key in required if implemented.get(key) is not True)
    ok = not missing and data.get("claim_allowed") is False and data.get("matrix", {}).get("release_claim_allowed") is False
    if not ok:
        return result(False, "INCOMPLETE", f"missing/false instrumentation: {missing}", [str(path.relative_to(root))])
    return result(True, "PROVEN_STRUCTURAL", "device probe, bundle and matrix instruments are implemented; physical evidence remains separate", [str(path.relative_to(root))])


def check_truth_sources(root: Path, contract: dict[str, Any]) -> dict[str, Any]:
    sources = contract.get("canonical_sources", {})
    optional = {"remote_ci_evidence"}
    missing = [value for key, value in sources.items() if key not in optional and not (root / value).exists()]
    if missing:
        return result(False, "MISSING", f"missing canonical sources: {missing}")
    return result(True, "PROVEN_STRUCTURAL", f"{len(sources) - len(optional)} mandatory canonical sources are present", sorted(value for key, value in sources.items() if key not in optional))


def check_remote_ci(root: Path) -> dict[str, Any]:
    path = root / "reports" / "ci-finalization-evidence.json"
    if not path.exists():
        return result(False, "TOKEN_VAZIO", "no finalization CI receipt with observable steps/logs", [str(path.relative_to(root))])
    data = load_json(path)
    ok = (
        data.get("status") == "PASS"
        and int(data.get("steps_observed", 0)) > 0
        and data.get("logs_available") is True
        and isinstance(data.get("head_sha"), str)
        and len(data.get("head_sha", "")) == 40
    )
    return result(ok, "PROVEN" if ok else "INVALID", "observable CI receipt accepted" if ok else "CI receipt does not prove steps, logs and immutable head", [str(path.relative_to(root))])


def find_domain(gap_map: dict[str, Any], domain_id: str) -> dict[str, Any] | None:
    return next((domain for domain in gap_map.get("domains", []) if domain.get("id") == domain_id), None)


def find_criterion(domain: dict[str, Any] | None, criterion_id: str) -> dict[str, Any] | None:
    if not domain:
        return None
    return next((item for item in domain.get("criteria", []) if item.get("id") == criterion_id), None)


def check_production_signing(root: Path) -> dict[str, Any]:
    path = root / "configs" / "first-part-gap-map.json"
    data = load_json(path)
    item = find_criterion(find_domain(data, "android.apk.pipeline"), "production_release_signing")
    ok = bool(item and item.get("state") == "PROVEN")
    return result(ok, "PROVEN" if ok else "TOKEN_VAZIO", "production signing evidence registered" if ok else "production release signing is not proven", [str(path.relative_to(root))])


def check_dual_arm_evidence(root: Path) -> dict[str, Any]:
    path = root / "reports" / "RAFAELIA_ZERO_OPERATIONAL_EVIDENCE_BASELINE_20260720.json"
    data = load_json(path)
    targets = data.get("required_physical_targets", {})
    matrix = data.get("matrix", {})
    roles = ("arm32-legacy", "arm64-modern")
    roles_ok = all(
        targets.get(role, {}).get("state") not in (None, "TOKEN_VAZIO")
        and targets.get(role, {}).get("selected_bundle")
        for role in roles
    )
    ok = roles_ok and matrix.get("state") == "DUAL_ARM_DEVICE_PROOF" and matrix.get("claim_allowed_device_matrix") is True
    return result(ok, "DUAL_ARM_DEVICE_PROOF" if ok else "TOKEN_VAZIO", "dual ARM matrix accepted" if ok else "ARM32 and ARM64 physical bundles have not both been selected and validated", [str(path.relative_to(root))])


def parse_truth_table(path: Path) -> dict[str, str]:
    states: dict[str, str] = {}
    for raw in path.read_text(encoding="utf-8").splitlines():
        line = raw.strip()
        if not line.startswith("|"):
            continue
        cells = [cell.strip().strip("`") for cell in line.strip("|").split("|")]
        if len(cells) >= 2 and cells[0] not in {"Recurso", "---"}:
            states[cells[0]] = cells[1]
    return states


def check_real_package_stack(root: Path) -> dict[str, Any]:
    path = root / "docs" / "RUNTIME_TRUTH_TABLE.md"
    states = parse_truth_table(path)
    required = ("payload ARM real", "pkg real", "apt", "apt-get", "dpkg", "libapt", "proot", "certificados", "DNS/network básico", "repositório configurado")
    not_proven = {name: states.get(name, "MISSING") for name in required if not states.get(name, "").startswith("PROVADO")}
    ok = not not_proven
    return result(ok, "PROVEN" if ok else "BLOCKED", "prefix-safe real package stack is proven" if ok else json.dumps(not_proven, ensure_ascii=False, sort_keys=True), [str(path.relative_to(root))])


def current_git_head(root: Path) -> str | None:
    completed = subprocess.run(["git", "rev-parse", "HEAD"], cwd=root, text=True, stdout=subprocess.PIPE, stderr=subprocess.DEVNULL, check=False)
    value = completed.stdout.strip()
    return value if completed.returncode == 0 and len(value) == 40 else None


def check_runtime_lock(root: Path) -> dict[str, Any]:
    path = root / "runtime-lock.json"
    data = load_json(path)
    primary = next((item for item in data.get("repositories", []) if item.get("name") == "termux-app-rafacodephi"), None)
    head = current_git_head(root)
    ok = bool(primary and primary.get("branch") == "master" and head and primary.get("commit") == head and primary.get("commit_hash_sha256") != "TOKEN_VAZIO")
    detail = "runtime lock matches immutable current head" if ok else f"runtime lock is not release-current (branch={primary.get('branch') if primary else None}, commit={primary.get('commit') if primary else None}, head={head})"
    return result(ok, "PROVEN" if ok else "STALE_OR_INCOMPLETE", detail, [str(path.relative_to(root))])


def check_research_domain(root: Path, kind: str) -> dict[str, Any]:
    path = root / "configs" / "first-part-gap-map.json"
    data = load_json(path)
    if kind == "browser_tls":
        ids = ("browser.tls12", "browser.tls13", "browser.tls.certification")
        criteria = [item for domain_id in ids for item in (find_domain(data, domain_id) or {}).get("criteria", [])]
        ok = bool(criteria) and all(item.get("state") == "PROVEN" for item in criteria)
        detail = "TLS 1.2/1.3 and certification are proven" if ok else "owned TLS and certification remain research gaps"
    elif kind == "complete_apkc_compilers":
        domain = find_domain(data, "apkc.compilers")
        required = ("complete_owned_compiler", "cross_language_ir", "dex_backend", "elf_backend", "apk_packager_end_to_end", "runtime_test_matrix")
        items = [find_criterion(domain, criterion) for criterion in required]
        ok = all(item and item.get("state") == "PROVEN" for item in items)
        detail = "complete APKC compiler chain is proven" if ok else "fixed ELF/DEX fixtures are not complete owned compilers"
    elif kind == "complete_vcpu_vm":
        receipt = root / "reports" / "vcpu-complete-runtime-evidence.json"
        ok = receipt.exists() and load_json(receipt).get("status") == "PASS" and load_json(receipt).get("claim_allowed") is True
        detail = "complete VCPU/VM runtime receipt accepted" if ok else "VCPU remains a deterministic state kernel, not a complete VM"
    else:
        raise ValueError(f"unknown research domain: {kind}")
    return result(ok, "PROVEN" if ok else "TOKEN_VAZIO", detail, [str(path.relative_to(root))])


def evaluate(root: Path, profile: str) -> dict[str, Any]:
    contract = load_json(root / "configs" / "system-finalization-contract.json")
    profiles = contract.get("profiles", {})
    if profile not in profiles:
        raise ValueError(f"unknown profile: {profile}")
    checks = {
        "build_metadata": check_build_metadata(root),
        "github_action_references": check_action_references(root),
        "loader_quarantine": check_loader_quarantine(root),
        "rafaelia_zero_instrumentation": check_zero_instrumentation(root),
        "canonical_truth_sources": check_truth_sources(root, contract),
        "observable_remote_ci": check_remote_ci(root),
        "production_release_signing": check_production_signing(root),
        "dual_arm_device_evidence": check_dual_arm_evidence(root),
        "prefix_safe_real_package_stack": check_real_package_stack(root),
        "current_federation_runtime_lock": check_runtime_lock(root),
        "browser_tls": check_research_domain(root, "browser_tls"),
        "complete_apkc_compilers": check_research_domain(root, "complete_apkc_compilers"),
        "complete_vcpu_vm": check_research_domain(root, "complete_vcpu_vm"),
    }
    profile_contract = profiles[profile]
    required = list(profile_contract.get("required_checks", []))
    failures = [name for name in required if not checks[name]["ok"]]
    closed = not failures
    release_allowed = bool(closed and profile_contract.get("release_allowed") is True)
    return {
        "schema": "termux.rafacodephi.system-finalization-report.v1",
        "profile": profile,
        "profile_closed": closed,
        "state": profile_contract.get("success_state") if closed else "BLOCKED",
        "claim_scope": profile_contract.get("claim_scope"),
        "claim_allowed_scope": closed,
        "release_allowed": release_allowed,
        "required_checks": required,
        "failed_required_checks": failures,
        "allowed_open_gaps": profile_contract.get("allowed_open_gaps", []),
        "checks": checks,
        "global_boundary": "safe-core closure never implies functional distribution release or full-platform completion",
    }


def markdown_report(report: dict[str, Any]) -> str:
    lines = [
        "# RAFCODEPHI system finalization",
        "",
        f"- Profile: `{report['profile']}`",
        f"- State: `{report['state']}`",
        f"- Profile closed: `{str(report['profile_closed']).lower()}`",
        f"- Scope claim allowed: `{str(report['claim_allowed_scope']).lower()}`",
        f"- Release allowed: `{str(report['release_allowed']).lower()}`",
        "",
        "| Check | Required | State | OK | Detail |",
        "|---|---|---|---:|---|",
    ]
    required = set(report["required_checks"])
    for name, item in report["checks"].items():
        detail = str(item["detail"]).replace("|", "\\|").replace("\n", " ")
        lines.append(f"| `{name}` | {'yes' if name in required else 'no'} | `{item['state']}` | {'yes' if item['ok'] else 'no'} | {detail} |")
    lines.extend(["", f"> {report['global_boundary']}", ""])
    return "\n".join(lines)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--root", default=str(ROOT))
    parser.add_argument("--profile", choices=("safe-core", "functional-distribution", "full-platform"), default="safe-core")
    parser.add_argument("--strict", action="store_true", help="return non-zero when the selected profile is not closed")
    parser.add_argument("--write-report", nargs="?", const="reports/system-finalization-report.json")
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    root = Path(args.root).resolve()
    report = evaluate(root, args.profile)
    print(markdown_report(report))
    if args.write_report:
        output = root / args.write_report
        output.parent.mkdir(parents=True, exist_ok=True)
        output.write_text(json.dumps(report, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    if args.strict and not report["profile_closed"]:
        print(f"FINALIZATION_PROFILE_BLOCKED={args.profile}", file=sys.stderr)
        return 1
    print(f"FINALIZATION_PROFILE_STATE={report['state']}", file=sys.stderr)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
