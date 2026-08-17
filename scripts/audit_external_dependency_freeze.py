#!/usr/bin/env python3
from __future__ import annotations

import json
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
LOCK = ROOT / "external" / "rafcodephi-build-freeze.lock.json"
WORKFLOW = ROOT / ".github" / "workflows" / "beta-build.yml"
WRAPPER = ROOT / "gradle" / "wrapper" / "gradle-wrapper.properties"
GRADLE = ROOT / "build.gradle"
PROPS = ROOT / "gradle.properties"
ANDROID_SETUP = ROOT / "scripts" / "setup_android_toolchain.sh"
ARTIFACT_RESTORE = ROOT / "scripts" / "restore_beta_lkg_artifact.sh"

def read(path: Path) -> str:
    if not path.is_file():
        raise SystemExit(f"FREEZE_AUDIT_MISSING_FILE={path.relative_to(ROOT)}")
    return path.read_text(encoding="utf-8")

def main() -> int:
    lock = json.loads(read(LOCK))
    workflow = read(WORKFLOW)
    wrapper = read(WRAPPER)
    gradle = read(GRADLE)
    props = read(PROPS)
    android = read(ANDROID_SETUP)
    artifact_restore = read(ARTIFACT_RESTORE)

    checks: list[tuple[str, bool, str]] = []
    def check(name: str, ok: bool, detail: str = "") -> None:
        checks.append((name, bool(ok), detail))

    termux = lock["termux_packages"]["commit"]
    builder_ref = lock["builder"]["reference"]
    actions = lock["github_actions"]

    check("schema", lock.get("schema") == "rafcodephi.build-freeze/v1")
    check("termux_pin_full_sha", bool(re.fullmatch(r"[0-9a-f]{40}", termux)), termux)
    check("termux_pin_workflow", workflow.count(termux) >= 3, f"occurrences={workflow.count(termux)}")
    check("builder_digest_full", bool(re.fullmatch(r"ghcr\.io/[^@]+@sha256:[0-9a-f]{64}", builder_ref)), builder_ref)
    check("builder_digest_workflow", builder_ref in workflow)
    check("java_exact_version", f"java-version: '{lock['java']['version']}'" in workflow)
    check("checkout_action_sha", f"actions/checkout@{actions['actions/checkout']}" in workflow)
    check("setup_java_action_sha", f"actions/setup-java@{actions['actions/setup-java']}" in workflow)
    check("cache_action_sha", workflow.count(f"actions/cache/restore@{actions['actions/cache']}") == 1 and
          workflow.count(f"actions/cache/save@{actions['actions/cache']}") == 1)
    check("upload_action_sha", f"actions/upload-artifact@{actions['actions/upload-artifact']}" in workflow)
    check("no_mutable_action_major_tags", re.search(r"uses:\s+actions/[^@\s]+@v\d+\b", workflow) is None)
    check("all_action_refs_are_sha", all(re.fullmatch(r"[0-9a-f]{40}", ref) for ref in
          re.findall(r"uses:\s+actions/[^@\s]+@([^\s#]+)", workflow)))

    gradle_sha = lock["gradle"]["distribution_sha256"]
    check("gradle_distribution_url", f"distributionUrl=https\\://services.gradle.org/distributions/gradle-{lock['gradle']['version']}-bin.zip" in wrapper)
    check("gradle_distribution_sha256", f"distributionSha256Sum={gradle_sha}" in wrapper)
    check("jitpack_removed", "jitpack.io" not in gradle)
    check("no_insecure_gradle_repo", "http://" not in gradle)
    check("no_dynamic_gradle_versions", not re.search(r'["\'][^"\']+:[^"\']*(?:\+|latest\.)[^"\']*["\']', gradle, re.I))

    a = lock["android"]
    check("compile_sdk_exact", f"compileSdkVersion={a['compile_sdk']}" in props)
    check("target_sdk_exact", f"targetSdkVersion={a['target_sdk']}" in props)
    check("build_tools_exact", f"buildToolsVersion={a['build_tools']}" in props)
    check("ndk_exact", f"ndkVersion={a['ndk']}" in props)
    check("cmdline_tools_version_exact", str(a["cmdline_tools_version"]) in android)
    check("cmdline_tools_linux_hash", a["cmdline_tools_linux_sha256"] in android)
    check("cmdline_tools_mac_hash", a["cmdline_tools_mac_sha256"] in android)
    check("cmdline_tools_hash_enforced", "CMDLINE_TOOLS_SHA256_MISMATCH" in android)
    check("android_download_host_closed", "https://dl.google.com/android/repository/" in android)
    check("cmake_exact", f'"cmake;{a["cmake"]}"' in android)

    check("pair_transaction_selftest", "test_bootstrap_pair_transaction.py" in workflow)
    check("lkg_cache_restore", "Restore last-known-good bootstrap pair" in workflow)
    check("lkg_artifact_restore", "Restore validated LKG from successful workflow artifact" in workflow and "gh run download" in artifact_restore)
    check("lkg_strict_arm_validation", "--arch arm --zip" in workflow and "--validate-only" in workflow)
    check("lkg_strict_aarch64_validation", "--arch aarch64 --zip" in workflow and "--validate-only" in workflow)
    check("resolver_primary", "state=PRIMARY" in workflow)
    check("resolver_failover", "state=FAILOVER_LKG" in workflow)
    check("resolver_blocked", "state=BLOCKED" in workflow)
    check("downstream_resolver_guard", workflow.count("steps.resolve_bootstrap.outcome == 'success'") >= 4)
    check("green_primary_distinct", "GREEN_PRIMARY" in workflow)
    check("green_degraded_distinct", "GREEN_DEGRADED" in workflow)
    check("legacy_prefix_forbidden", "legacy_prefix_allowed=false" in workflow and lock["policy"]["legacy_prefix_allowed"] is False)
    check("bridge_forbidden", "bridge_allowed=false" in workflow and lock["policy"]["bridge_allowed"] is False)
    check("physical_android_not_promoted", "PHYSICAL_ANDROID: 'TOKEN_VAZIO'" in workflow and lock["runtime"]["physical_android"] == "TOKEN_VAZIO")
    check("claim_fail_closed", "CLAIM_ALLOWED: 'false'" in workflow and lock["runtime"]["claim_allowed"] is False)
    check("pss3_no_trace_non_blocker", "NO_FAILURE_TRACE: optional PSS3 audit has no failure trace to evaluate" in workflow)

    verification_file = ROOT / "gradle" / "verification-metadata.xml"
    evidence_gaps = []
    if not verification_file.is_file():
        evidence_gaps.append("TOKEN_VAZIO_GRADLE_DEPENDENCY_VERIFICATION_REQUIRES_REAL_RESOLUTION_RECEIPT")
    if lock["runtime"]["physical_android"] == "TOKEN_VAZIO":
        evidence_gaps.append("TOKEN_VAZIO_PHYSICAL_ANDROID_ARM32_ARM64")

    failed = [{"name": n, "detail": d} for n, ok, d in checks if not ok]
    report = {
        "schema": "rafcodephi.external-dependency-freeze-audit/v1",
        "state": "PASS" if not failed else "BLOCKED",
        "checks_total": len(checks),
        "checks_passed": len(checks) - len(failed),
        "checks_failed": failed,
        "evidence_gaps_non_promoting": evidence_gaps,
        "claim_allowed": False,
        "physical_android": "TOKEN_VAZIO",
    }
    out = ROOT / "build" / "reports" / "external-dependency-freeze.json"
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text(json.dumps(report, sort_keys=True, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(report, sort_keys=True))
    if failed:
        for item in failed:
            print(f"FREEZE_AUDIT_FAIL={item['name']} {item['detail']}", file=sys.stderr)
        return 1
    print(f"EXTERNAL_DEPENDENCY_FREEZE=PASS checks={len(checks)} evidence_gaps={len(evidence_gaps)}")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
