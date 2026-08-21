#!/usr/bin/env python3
"""Generate a fail-closed semantic custody receipt for the RAFCODEPHI usable beta.

This receipt intentionally binds source identity -> source-built bootstraps -> APK
artifacts while keeping physical Android execution as TOKEN_VAZIO until an exact
APK hash is installed and exercised on a device.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
import re
from pathlib import Path
from typing import Any

SCHEMA = "rafcodephi.usable-beta-build/v2"
PACKAGE_NAME = "com.termux.rafacodephi"
PREFIX = "/data/data/com.termux.rafacodephi/files/usr"
EXPECTED_ABIS = ("armeabi-v7a", "arm64-v8a")
ARCH_BY_ABI = {"armeabi-v7a": "arm", "arm64-v8a": "aarch64"}
EXPECTED_BOOTSTRAP_SCHEMA = "rafcodephi.real-bootstrap-sourcebuild/v1"
EXPECTED_PROFILE_SCHEMA = "rafcodephi-bootstrap-profile/v1"


class ReceiptError(RuntimeError):
    pass


def fail(message: str) -> None:
    raise ReceiptError(message)


def sha256_file(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as fh:
        for chunk in iter(lambda: fh.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()


def read_json(path: Path) -> dict[str, Any]:
    if not path.is_file():
        fail(f"missing JSON evidence: {path}")
    try:
        value = json.loads(path.read_text())
    except (OSError, json.JSONDecodeError) as exc:
        fail(f"invalid JSON evidence {path}: {exc}")
    if not isinstance(value, dict):
        fail(f"JSON evidence must be an object: {path}")
    return value


def parse_key_value(path: Path) -> dict[str, str]:
    if not path.is_file():
        fail(f"missing key/value evidence: {path}")
    out: dict[str, str] = {}
    for raw in path.read_text(errors="strict").splitlines():
        line = raw.strip()
        if not line or line.startswith("#") or "=" not in line:
            continue
        key, value = line.split("=", 1)
        out[key.strip()] = value.strip().strip('"')
    return out


def require_equal(actual: Any, expected: Any, label: str) -> None:
    if actual != expected:
        fail(f"{label}: expected={expected!r} actual={actual!r}")


def validate_profile(report: dict[str, Any], *, arch: str) -> dict[str, Any]:
    require_equal(report.get("schema"), EXPECTED_PROFILE_SCHEMA, f"{arch}.schema")
    require_equal(report.get("profile"), "real-pkg", f"{arch}.profile")
    require_equal(report.get("arch"), arch, f"{arch}.arch")
    require_equal(report.get("package_name"), PACKAGE_NAME, f"{arch}.package_name")
    require_equal(report.get("structural_state"), "PASS", f"{arch}.structural_state")
    require_equal(report.get("device_validation"), "TOKEN_VAZIO", f"{arch}.device_validation")
    require_equal(report.get("claim_allowed"), False, f"{arch}.claim_allowed")
    require_equal(report.get("release_allowed"), False, f"{arch}.release_allowed")
    classifications = report.get("classifications")
    if not isinstance(classifications, dict):
        fail(f"{arch}.classifications missing")
    for binary in ("bin/apt", "bin/apt-get"):
        require_equal(classifications.get(binary), "ELF", f"{arch}.{binary}")
    zip_sha = report.get("zip_sha256")
    if not isinstance(zip_sha, str) or not re.fullmatch(r"[0-9a-f]{64}", zip_sha):
        fail(f"{arch}.zip_sha256 invalid: {zip_sha!r}")
    return {
        "arch": arch,
        "zip": report.get("zip"),
        "zip_sha256": zip_sha,
        "profile": report.get("profile"),
        "package_name": report.get("package_name"),
        "structural_state": report.get("structural_state"),
        "classifications": classifications,
        "device_validation": report.get("device_validation"),
        "claim_allowed": report.get("claim_allowed"),
        "release_allowed": report.get("release_allowed"),
    }


def apk_abi(path: Path) -> str:
    name = path.name
    for abi in EXPECTED_ABIS:
        if abi in name:
            return abi
    if "universal" in name:
        return "universal"
    return "unknown"


def apk_kind(path: Path) -> str:
    return "signed" if "signed" in path.parts else "unsigned"


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--apk-root", type=Path, default=Path("dist/apk-matrix"))
    parser.add_argument("--bootstrap-manifest", type=Path, required=True)
    parser.add_argument("--arm-report", type=Path, required=True)
    parser.add_argument("--arm64-report", type=Path, required=True)
    parser.add_argument("--source-contract", type=Path, required=True)
    parser.add_argument("--packages-sha", required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()

    packages_sha = args.packages_sha.strip()
    if not re.fullmatch(r"[0-9a-f]{40}", packages_sha):
        fail(f"packages SHA must be an exact 40-char commit: {packages_sha!r}")

    manifest = parse_key_value(args.bootstrap_manifest)
    require_equal(manifest.get("schema"), EXPECTED_BOOTSTRAP_SCHEMA, "bootstrap_manifest.schema")
    require_equal(manifest.get("package_name"), PACKAGE_NAME, "bootstrap_manifest.package_name")
    require_equal(manifest.get("prefix"), PREFIX, "bootstrap_manifest.prefix")
    require_equal(manifest.get("bridge_allowed"), "false", "bootstrap_manifest.bridge_allowed")
    require_equal(manifest.get("legacy_prefix_allowed"), "false", "bootstrap_manifest.legacy_prefix_allowed")

    source = parse_key_value(args.source_contract)
    require_equal(source.get("TERMUX_PACKAGES_RAF_REF"), packages_sha, "source_contract.requested_ref")
    require_equal(source.get("TERMUX_PACKAGES_RAF_RESOLVED_COMMIT"), packages_sha, "source_contract.resolved_commit")
    source_abis = tuple(x for x in source.get("TERMUX_PACKAGES_RAF_ABIS", "").split(",") if x)
    require_equal(source_abis, EXPECTED_ABIS, "source_contract.abis")
    require_equal(source.get("TERMUX_PACKAGES_RAF_BINARIES_CREATED_BY_CI_ONLY"), "1", "source_contract.ci_only")

    arm = validate_profile(read_json(args.arm_report), arch="arm")
    arm64 = validate_profile(read_json(args.arm64_report), arch="aarch64")
    bootstraps = {"arm": arm, "aarch64": arm64}

    if not args.apk_root.is_dir():
        fail(f"APK root missing: {args.apk_root}")
    apks: list[dict[str, Any]] = []
    install_candidates: dict[str, list[dict[str, Any]]] = {abi: [] for abi in EXPECTED_ABIS}
    for path in sorted(args.apk_root.rglob("*.apk")):
        abi = apk_abi(path)
        kind = apk_kind(path)
        item = {
            "path": str(path),
            "bytes": path.stat().st_size,
            "sha256": sha256_file(path),
            "abi": abi,
            "kind": kind,
            "release": "release" in path.name.lower(),
        }
        apks.append(item)
        if kind == "signed" and item["release"] and abi in install_candidates:
            install_candidates[abi].append(item.copy())

    if not apks:
        fail("APK matrix produced no APKs")
    for abi in EXPECTED_ABIS:
        if not install_candidates[abi]:
            fail(f"no signed release install candidate for required ABI {abi}")

    selection_state = {
        abi: "UNIQUE" if len(items) == 1 else "MULTIPLE_CANDIDATES_REQUIRE_EXACT_SHA_SELECTION"
        for abi, items in install_candidates.items()
    }

    sha_manifest = args.apk_root / "SHA256SUMS.txt"
    sha_manifest_receipt = None
    if sha_manifest.is_file():
        sha_manifest_receipt = {
            "path": str(sha_manifest),
            "sha256": sha256_file(sha_manifest),
        }

    provenance_edges: list[dict[str, str]] = []
    for abi, arch in ARCH_BY_ABI.items():
        bootstrap_sha = bootstraps[arch]["zip_sha256"]
        provenance_edges.append({
            "from": f"termux-packages@{packages_sha}",
            "relation": "SOURCE_BUILDS_BOOTSTRAP",
            "to": f"bootstrap:{arch}@sha256:{bootstrap_sha}",
        })
        for candidate in install_candidates[abi]:
            provenance_edges.append({
                "from": f"bootstrap:{arch}@sha256:{bootstrap_sha}",
                "relation": "EMBEDDED_FOR_ABI_IN_APK_BUILD",
                "to": f"apk:{candidate['path']}@sha256:{candidate['sha256']}",
            })

    doc = {
        "schema": SCHEMA,
        "state": "STRUCTURAL_PASS_DEVICE_TOKEN_VAZIO",
        "app_commit_sha": os.environ.get("GITHUB_SHA", "TOKEN_VAZIO"),
        "semantic_identity": {
            "package_name": PACKAGE_NAME,
            "prefix": PREFIX,
            "required_abis": list(EXPECTED_ABIS),
            "bootstrap_arch_by_abi": ARCH_BY_ABI,
        },
        "source_contract": {
            "repository": source.get("TERMUX_PACKAGES_RAF_REPO"),
            "requested_ref": source.get("TERMUX_PACKAGES_RAF_REF"),
            "resolved_commit": source.get("TERMUX_PACKAGES_RAF_RESOLVED_COMMIT"),
            "role": source.get("TERMUX_PACKAGES_RAF_ROLE"),
            "source_kind": source.get("TERMUX_PACKAGES_RAF_SOURCE_KIND"),
            "abis": list(source_abis),
            "ci_binaries_only": True,
        },
        "bootstrap_manifest": {
            "path": str(args.bootstrap_manifest),
            "sha256": sha256_file(args.bootstrap_manifest),
            "schema": manifest.get("schema"),
            "package_name": manifest.get("package_name"),
            "prefix": manifest.get("prefix"),
            "bridge_allowed": False,
            "legacy_prefix_allowed": False,
        },
        "bootstraps": bootstraps,
        "apk_matrix": {
            "root": str(args.apk_root),
            "apk_count": len(apks),
            "apks": apks,
            "sha256_manifest": sha_manifest_receipt,
        },
        "physical_install_candidates": install_candidates,
        "candidate_selection_state": selection_state,
        "provenance_edges": provenance_edges,
        "evidence_boundary": {
            "source_identity": "PASS",
            "bootstrap_structure": "PASS",
            "apt_elf": "PASS",
            "apk_artifacts": "PASS",
            "physical_android": "TOKEN_VAZIO",
            "device_runtime_proof": "TOKEN_VAZIO",
            "claim_allowed_device_runtime": False,
            "release_allowed": False,
        },
        "claim_allowed": False,
        "release_allowed": False,
    }

    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(doc, indent=2, sort_keys=True) + "\n")
    print(json.dumps(doc, indent=2, sort_keys=True))
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except ReceiptError as exc:
        print(json.dumps({"schema": SCHEMA, "state": "FAIL", "error": str(exc)}, sort_keys=True))
        raise SystemExit(1)
