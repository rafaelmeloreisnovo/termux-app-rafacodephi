#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import sys
import zipfile
from collections import Counter
from pathlib import Path

REQUIRED = (
    "bin/sh", "bin/bash", "bin/apt", "bin/apt-get", "bin/dpkg", "bin/pkg",
    "bin/proot", "bin/proot.real", "bin/cat", "bin/ls", "bin/clear", "bin/grep",
    "etc/apt/sources.list", "etc/resolv.conf", "etc/rafcodephi-core.env",
    "BOOTSTRAP_INFO", "SYMLINKS.txt",
)
PREFIX = "/data/data/com.termux.rafacodephi/files/usr"
LEGACY_PREFIXES = (
    b"/data/data/com.termux/files/usr",
    b"/data/data/com.termux/",
)
BINARY_RISK = "LEGACY_PREFIX_BINARY_RISK"
TEXT_RISK = "LEGACY_PREFIX_TEXT"
SCHEMA = "rafcodephi-real-pkg-prefix-audit/v1"
MATRIX_SCHEMA = "rafcodephi-real-pkg-prefix-audit-matrix/v2"


def decode_utf8(data: bytes) -> str | None:
    if b"\x00" in data:
        return None
    try:
        return data.decode("utf-8")
    except UnicodeDecodeError:
        return None


def finding(kind: str, *, entry: str = "", detail: str = "", legacy_prefix: str = "") -> dict:
    return {
        "kind": kind,
        "entry": entry,
        "legacy_prefix": legacy_prefix,
        "detail": detail,
    }


def classify_legacy_prefix(entry: str, data: bytes) -> list[dict]:
    results: list[dict] = []
    found = [prefix for prefix in LEGACY_PREFIXES if prefix in data]
    if not found:
        return results
    text = decode_utf8(data)
    for prefix in found:
        legacy = prefix.decode("utf-8")
        if text is None:
            results.append(finding(
                BINARY_RISK,
                entry=entry,
                legacy_prefix=legacy,
                detail=(
                    "recommendation=rebuild package with RAFCODEΦ prefix or use a safe compatibility strategy; "
                    "no binary replacement was performed"
                ),
            ))
        else:
            results.append(finding(
                TEXT_RISK,
                entry=entry,
                legacy_prefix=legacy,
                detail="text payload still contains the upstream com.termux prefix",
            ))
    return results


def check(path: Path) -> list[dict]:
    findings: list[dict] = []
    with zipfile.ZipFile(path) as zf:
        names = set(zf.namelist())
        symlink_destinations = set()
        if "SYMLINKS.txt" in names:
            for line in zf.read("SYMLINKS.txt").decode("utf-8", "replace").splitlines():
                parts = line.split("←")
                if len(parts) == 2:
                    symlink_destinations.add(parts[1])
        present = names | symlink_destinations
        for req in REQUIRED:
            if req not in present:
                findings.append(finding("MISSING_REQUIRED_ENTRY", entry=req, detail="required bootstrap entry missing"))

        info = zf.read("BOOTSTRAP_INFO").decode("utf-8", "replace") if "BOOTSTRAP_INFO" in names else ""
        for token in [
            "BOOTSTRAP_REAL_APT_READY=1",
            "BOOTSTRAP_REAL_DPKG_READY=1",
            "BOOTSTRAP_REAL_PROOT_READY=1",
            "BOOTSTRAP_REAL_COREUTILS_READY=1",
            "BOOTSTRAP_CA_CERTIFICATES_READY=1",
            "BOOTSTRAP_DNS_RESOLVER_READY=1",
            "BOOTSTRAP_MINIMUM_COMMANDS_READY=1",
        ]:
            if token not in info:
                findings.append(finding("MISSING_BOOTSTRAP_INFO_TOKEN", detail=token))

        for name in names:
            if name.startswith("/") or ".." in name.split("/"):
                findings.append(finding("UNSAFE_ZIP_ENTRY", entry=name, detail="unsafe zip entry: absolute/traversal path forbidden"))

        for name in sorted(names):
            if name.endswith("/"):
                continue
            findings.extend(classify_legacy_prefix(name, zf.read(name)))

        text_names = [
            n for n in names
            if n.endswith((".list", ".env", ".sh"))
            or n in ("bin/pkg", "bin/proot", "bin/cat", "bin/ls", "bin/clear", "bin/grep", "etc/resolv.conf")
        ]
        for name in text_names:
            data = zf.read(name)
            text = decode_utf8(data)
            if text is None:
                continue
            if name in ("etc/rafcodephi-core.env", "bin/proot", "bin/cat", "bin/ls", "bin/clear", "bin/grep") and PREFIX not in text:
                findings.append(finding(
                    "CANONICAL_PREFIX_MISSING",
                    entry=name,
                    detail=f"expected canonical prefix {PREFIX}",
                ))
    return findings


def classify_state(findings: list[dict]) -> tuple[str, str]:
    if not findings:
        return "PASS", "PREFIX_AND_STRUCTURE_PREDICATES_SATISFIED"
    kinds = {item["kind"] for item in findings}
    structural = kinds - {BINARY_RISK, TEXT_RISK}
    if structural:
        return "FAIL", "STRUCTURAL_BOOTSTRAP_CONTRACT_FAILURE"
    return "BLOCKED", "UPSTREAM_BINARY_PREFIX_REBUILD_REQUIRED"


def classify_matrix_state(reports: list[dict]) -> tuple[str, str]:
    """Preserve PASS/BLOCKED/FAIL semantics across multi-artifact audits.

    BLOCKED is an expected fail-closed observation and must never be silently
    rewritten to FAIL. Any real FAIL dominates the matrix. PASS is allowed only
    when every constituent report is PASS.
    """
    states = {str(report.get("state", "FAIL")) for report in reports}
    if states == {"PASS"}:
        return "PASS", "ALL_ARTIFACTS_PREFIX_AND_STRUCTURE_PASS"
    if "FAIL" in states or not states.issubset({"PASS", "BLOCKED"}):
        return "FAIL", "AT_LEAST_ONE_ARTIFACT_FAILED_AUDIT"
    return "BLOCKED", "AT_LEAST_ONE_ARTIFACT_REQUIRES_PREFIX_REBUILD"


def audit(path: Path) -> dict:
    findings = check(path)
    state, reason = classify_state(findings)
    counts = Counter(item["kind"] for item in findings)
    binary_entries = sorted({item["entry"] for item in findings if item["kind"] == BINARY_RISK})
    text_entries = sorted({item["entry"] for item in findings if item["kind"] == TEXT_RISK})
    return {
        "schema": SCHEMA,
        "zip": str(path),
        "state": state,
        "reason": reason,
        "canonical_prefix": PREFIX,
        "legacy_prefixes": [p.decode("utf-8") for p in LEGACY_PREFIXES],
        "finding_count": len(findings),
        "finding_counts": dict(sorted(counts.items())),
        "binary_risk_entry_count": len(binary_entries),
        "binary_risk_entries": binary_entries,
        "text_risk_entry_count": len(text_entries),
        "text_risk_entries": text_entries,
        "findings": findings,
        "claim_allowed_structural_real_pkg": state == "PASS",
        "claim_allowed_device_runtime": False,
        "claim_allowed_pkg_runtime": False,
        "release_allowed": False,
        "device_validation": "TOKEN_VAZIO",
        "token_vazio": [] if state == "PASS" else [
            {
                "id": "TV_REAL_PKG_PREFIX_REBUILD",
                "priority": "P0",
                "state": "TOKEN_VAZIO",
                "blocks": [
                    "real_pkg_profile_materialization",
                    "real_pkg_apk_candidate",
                    "device_pkg_smoke",
                    "release_allowed",
                ],
                "closure": (
                    "Rebuild every affected package against /data/data/com.termux.rafacodephi/files/usr "
                    "or provide a separately validated compatibility layer. Binary in-place prefix replacement is forbidden."
                ),
            }
        ],
        "next_required_action": (
            "REBUILD_AFFECTED_PACKAGES_FOR_RAFCODEPHI_PREFIX"
            if state == "BLOCKED"
            else "FIX_STRUCTURAL_BOOTSTRAP_FINDINGS"
            if state == "FAIL"
            else "MATERIALIZE_HASH_BOUND_REAL_PKG_PROFILE"
        ),
    }


def human_diagnostic(report: dict) -> list[str]:
    path = report.get("zip", "bootstrap.zip")
    lines: list[str] = []
    for item in report.get("findings", []):
        kind = item.get("kind", "UNKNOWN")
        entry = item.get("entry", "")
        legacy = item.get("legacy_prefix", "")
        detail = item.get("detail", "")
        if kind == BINARY_RISK:
            lines.append(f"{path}: {kind}: entry={entry} legacy_prefix={legacy} {detail}")
        elif kind == TEXT_RISK:
            lines.append(f"{path}: legacy prefix in text entry={entry} legacy_prefix={legacy}")
        elif kind == "UNSAFE_ZIP_ENTRY":
            lines.append(f"{path}: unsafe zip entry {entry}")
        elif entry:
            lines.append(f"{path}: {kind}: entry={entry} {detail}".rstrip())
        else:
            lines.append(f"{path}: {kind}: {detail}".rstrip())
    return lines


def write_report(path: Path, payload: dict) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(payload, ensure_ascii=False, sort_keys=True, indent=2) + "\n", encoding="utf-8")


def parse_args(argv: list[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("paths", nargs="+")
    parser.add_argument("--json", dest="json_path", type=Path)
    return parser.parse_args(argv)


def main(argv: list[str]) -> int:
    args = parse_args(argv)
    reports = []
    for raw in args.paths:
        path = Path(raw)
        if not path.exists():
            reports.append({
                "schema": SCHEMA,
                "zip": str(path),
                "state": "FAIL",
                "reason": "MISSING_ZIP",
                "finding_count": 1,
                "findings": [finding("MISSING_ZIP", entry=str(path), detail="bootstrap zip does not exist")],
                "claim_allowed_structural_real_pkg": False,
                "claim_allowed_device_runtime": False,
                "claim_allowed_pkg_runtime": False,
                "release_allowed": False,
                "device_validation": "TOKEN_VAZIO",
                "token_vazio": [],
                "next_required_action": "CREATE_BOOTSTRAP_ZIP",
            })
            continue
        try:
            reports.append(audit(path))
        except (OSError, zipfile.BadZipFile, KeyError, UnicodeDecodeError) as exc:
            reports.append({
                "schema": SCHEMA,
                "zip": str(path),
                "state": "FAIL",
                "reason": "AUDIT_EXCEPTION",
                "finding_count": 1,
                "findings": [finding("AUDIT_EXCEPTION", detail=str(exc))],
                "claim_allowed_structural_real_pkg": False,
                "claim_allowed_device_runtime": False,
                "claim_allowed_pkg_runtime": False,
                "release_allowed": False,
                "device_validation": "TOKEN_VAZIO",
                "token_vazio": [],
                "next_required_action": "FIX_AUDIT_EXCEPTION",
            })

    if len(reports) == 1:
        payload = reports[0]
    else:
        matrix_state, matrix_reason = classify_matrix_state(reports)
        payload = {
            "schema": MATRIX_SCHEMA,
            "reports": reports,
            "state": matrix_state,
            "reason": matrix_reason,
            "claim_allowed_structural_real_pkg": matrix_state == "PASS",
            "claim_allowed_device_runtime": False,
            "claim_allowed_pkg_runtime": False,
            "release_allowed": False,
        }
    if args.json_path:
        write_report(args.json_path, payload)
    print(json.dumps(payload, ensure_ascii=False, sort_keys=True, indent=2))

    for report in reports:
        if report.get("state") != "PASS":
            for line in human_diagnostic(report):
                print(line, file=sys.stderr)

    states = {report["state"] for report in reports}
    if states == {"PASS"}:
        print("real_arm_bootstrap_core=PASS")
        return 0
    # BLOCKED and FAIL both stop promotion. JSON state/reason preserve the distinction.
    return 1


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
