#!/usr/bin/env python3
"""Fail closed unless the loader is either a proven inert stub or fully security-gated."""
from __future__ import annotations

import argparse
import json
from pathlib import Path
from typing import Mapping

ROOT = Path(__file__).resolve().parents[1]
REPORT = Path("dist/loader/loader-functional-security.json")
FILES = {
    "contract": "configs/loader-functional-security-contract.json",
    "manifest": "app/loader/src/main/AndroidManifest.xml",
    "readme": "app/loader/README.md",
    "loader_gradle": "app/loader/build.gradle",
    "workflow": ".github/workflows/loader-apk-contract.yml",
    "artifact_verifier": "scripts/verify_loader_apk.sh",
}
OPTIONAL = {
    "install_contract": "app/loader/src/main/java/com/termux/rafacodephi/loader/BootstrapInstallContract.java",
    "activity": "app/loader/src/main/java/com/termux/rafacodephi/loader/LoaderActivity.java",
    "service": "app/loader/src/main/java/com/termux/rafacodephi/loader/BootstrapInstallService.java",
    "source_policy": "app/loader/src/main/java/com/termux/rafacodephi/loader/BootstrapSourcePolicy.java",
    "provider": "app/loader/src/main/java/com/termux/rafacodephi/loader/VerifiedBootstrapProvider.java",
    "host_client": "app/src/main/java/com/termux/app/BootstrapLoaderClient.java",
    "host_receiver": "app/src/main/java/com/termux/app/BootstrapHandoffReceiver.java",
    "host_gate": "app/src/main/java/com/termux/app/BootstrapGateActivity.java",
    "host_integrity": "app/src/main/java/com/termux/app/BootstrapIntegrityVerifier.java",
    "host_manifest": "app/src/main/AndroidManifest.xml",
    "native": "app/src/main/cpp/termux-bootstrap.c",
    "pin_gradle": "app/build.gradle",
}


def load(root: Path = ROOT) -> dict[str, str]:
    result: dict[str, str] = {}
    for key, relative in FILES.items():
        result[key] = (root / relative).read_text(encoding="utf-8")
    for key, relative in OPTIONAL.items():
        path = root / relative
        result[key] = path.read_text(encoding="utf-8") if path.is_file() else ""
    return result


def contains(text: str, *needles: str) -> bool:
    return all(needle in text for needle in needles)


def validate_snapshot(files: Mapping[str, str]) -> tuple[str, list[str]]:
    errors: list[str] = []
    contract = json.loads(files["contract"])
    manifest = files["manifest"]
    readme = files["readme"]
    gradle = files["loader_gradle"]
    workflow = files["workflow"]
    verifier = files["artifact_verifier"]

    loader_sources = [
        files["install_contract"], files["activity"], files["service"],
        files["source_policy"], files["provider"],
    ]
    java_present = any(source.strip() for source in loader_sources)
    has_code_false = 'android:hasCode="false"' in manifest

    if contract.get("claim_allowed") is not False or contract.get("release_allowed") is not False:
        errors.append("contract must keep claim and release disabled")
    if set(contract.get("allowed_states", [])) != {"STUB_SAFE_BLOCKED", "FUNCTIONAL_SECURITY_GATED"}:
        errors.append("contract must allow exactly two states")

    # The loader must never extract the bootstrap. The host is the custody
    # boundary and is expected to inspect ZIP entries before atomic install.
    combined_source = "\n".join(loader_sources)
    for token in contract.get("forbidden_source_tokens", []):
        if token in combined_source:
            errors.append(f"forbidden source token present: {token}")

    if not java_present:
        state = "STUB_SAFE_BLOCKED"
        if not has_code_false:
            errors.append("stub must declare android:hasCode=false")
        if not contains(manifest, "STUB_NO_BOOTSTRAP_PAYLOAD", 'android:debuggable="false"'):
            errors.append("stub manifest identity is incomplete")
        if not contains(readme, "has_code = false", "installer_behavior = absent", "release_allowed = false", "BLOCKED_BY[LOADER_FUNCTIONAL_CONTRACT_REQUIRED]"):
            errors.append("stub README does not preserve blocked state")
        if not contains(gradle, 'versionName "0.1.0-stub"', "Builds the loader stub"):
            errors.append("stub Gradle identity is incomplete")
        if not contains(verifier, "state=STUB_NO_BOOTSTRAP_PAYLOAD", "manifest_has_code_false", "dex_policy=no_dex"):
            errors.append("artifact verifier no longer proves inert stub")
        if "FUNCTIONAL_SECURITY_GATED" in readme or "functional_installer = true" in readme:
            errors.append("stub documentation claims functional capability")
    else:
        state = "FUNCTIONAL_SECURITY_GATED"
        if has_code_false:
            errors.append("hybrid state: Java sources exist while manifest disables code")
        required_nonempty = (
            "install_contract", "activity", "service", "source_policy", "provider",
            "host_client", "host_receiver", "host_gate", "host_integrity",
            "host_manifest", "native", "pin_gradle",
        )
        for key in required_nonempty:
            if not files[key].strip():
                errors.append(f"functional boundary file missing: {key}")

        permission = "com.termux.rafacodephi.permission.BOOTSTRAP_HANDOFF"
        if not contains(manifest, 'android:usesCleartextTraffic="false"',
                        f'android:permission="{permission}"',
                        'android:exported="false"',
                        'android:grantUriPermissions="true"',
                        "BOOTSTRAP_ACQUIRE_HANDOFF_CAPABLE"):
            errors.append("functional loader manifest boundary incomplete")
        if "http://" in "\n".join(loader_sources):
            errors.append("functional source contains plaintext HTTP")
        if not contains(files["source_policy"], '"https"', "MAX_REDIRECTS", "sameOrigin",
                        "NON_STANDARD_HTTPS_PORT_BLOCKED", "CROSS_ORIGIN_REDIRECT_BLOCKED"):
            errors.append("HTTPS same-origin policy incomplete")
        if not contains(files["service"], "MAX_DOWNLOAD_BYTES", "setInstanceFollowRedirects(false)",
                        "SHA256_MISMATCH", "getFD().sync()", "grantUriPermission", "setPackage"):
            errors.append("bounded acquisition and explicit handoff incomplete")
        if not contains(files["provider"], "MODE_READ_ONLY", "READ_ONLY_PROVIDER", "getCanonicalPath"):
            errors.append("provider must be private and read-only")
        if not contains(files["host_client"], "checkSignatures", "SIGNATURE_MATCH",
                        "EXTERNAL_CANONICAL_BLAKE3_NOT_CONFIGURED"):
            errors.append("host signer and canonical BLAKE3 gate incomplete")
        if not contains(files["host_receiver"], 'MessageDigest.getInstance("SHA-256")',
                        "blake3Hex", "MAX_ZIP_ENTRIES", "MAX_UNCOMPRESSED_BYTES",
                        "revokeUriPermission", "Os.rename"):
            errors.append("host double-hash ZIP custody incomplete")
        if not contains(files["host_manifest"], 'android:protectionLevel="signature"',
                        "BootstrapHandoffReceiver", "BootstrapGateActivity"):
            errors.append("host manifest signature custody incomplete")
        if not contains(files["native"], "O_NOFOLLOW", "errno == ENOENT", "S_ISREG"):
            errors.append("native private-inbox fallback boundary incomplete")
        lower_native = files["native"].lower()
        if any(token in lower_native for token in ("malloc(", "calloc(", "realloc(")):
            errors.append("native loader path allocates heap")
        pin_envs = (
            "TERMUX_EXTERNAL_BOOTSTRAP_URL_AARCH64",
            "TERMUX_EXTERNAL_BOOTSTRAP_URL_ARM",
            "TERMUX_EXTERNAL_BOOTSTRAP_URL_I686",
            "TERMUX_EXTERNAL_BOOTSTRAP_URL_X86_64",
            "TERMUX_EXTERNAL_BOOTSTRAP_SHA256_AARCH64",
            "TERMUX_EXTERNAL_BOOTSTRAP_SHA256_ARM",
            "TERMUX_EXTERNAL_BOOTSTRAP_SHA256_I686",
            "TERMUX_EXTERNAL_BOOTSTRAP_SHA256_X86_64",
        )
        for env in pin_envs:
            if env not in files["pin_gradle"] or f'System.getenv("{env}") ?: ""' not in files["pin_gradle"]:
                errors.append(f"external pin must exist and default empty: {env}")
        signing = (
            "TERMUX_ENABLE_RELEASE_SIGNING", "TERMUX_RELEASE_KEYSTORE_FILE",
            "TERMUX_RELEASE_KEYSTORE_PASSWORD", "TERMUX_RELEASE_KEY_ALIAS",
            "TERMUX_RELEASE_KEY_PASSWORD",
        )
        if not all(item in gradle for item in signing):
            errors.append("loader release signing is not tied to host inputs")

    if not contains(workflow, "validate_loader_functional_security.py", "test_loader_functional_security.py"):
        errors.append("canonical loader workflow does not execute quarantine gate")
    return state, errors


def report(root: Path = ROOT) -> dict[str, object]:
    state, errors = validate_snapshot(load(root))
    return {
        "schema": "termux.rafacodephi.loader-functional-security-validation.v1",
        "status": "PASS" if not errors else "FAIL",
        "state": state,
        "functional_capability": state == "FUNCTIONAL_SECURITY_GATED" and not errors,
        "claim_allowed": False,
        "release_allowed": False,
        "errors": errors,
        "next_gate": "Full artifact, signer and Android device evidence is required before functional or release claims.",
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--root", type=Path, default=ROOT)
    parser.add_argument("--strict", action="store_true")
    parser.add_argument("--write-report", action="store_true")
    args = parser.parse_args()
    result = report(args.root)
    text = json.dumps(result, ensure_ascii=False, sort_keys=True, indent=2) + "\n"
    if args.write_report:
        path = args.root / REPORT
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(text, encoding="utf-8")
    print(text, end="")
    return 1 if args.strict and result["status"] != "PASS" else 0


if __name__ == "__main__":
    raise SystemExit(main())
