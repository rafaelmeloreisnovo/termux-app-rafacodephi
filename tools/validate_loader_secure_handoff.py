#!/usr/bin/env python3
"""Static, fail-closed validator for the loader/host bootstrap handoff v2."""
from __future__ import annotations

import argparse
import json
import re
import sys
from pathlib import Path
from typing import Mapping

ROOT = Path(__file__).resolve().parents[1]
FILES = {
    "loader_manifest": "app/loader/src/main/AndroidManifest.xml",
    "host_manifest": "app/src/main/AndroidManifest.xml",
    "contract": "app/loader/src/main/java/com/termux/rafacodephi/loader/BootstrapInstallContract.java",
    "activity": "app/loader/src/main/java/com/termux/rafacodephi/loader/LoaderActivity.java",
    "service": "app/loader/src/main/java/com/termux/rafacodephi/loader/BootstrapInstallService.java",
    "policy": "app/loader/src/main/java/com/termux/rafacodephi/loader/BootstrapSourcePolicy.java",
    "provider": "app/loader/src/main/java/com/termux/rafacodephi/loader/VerifiedBootstrapProvider.java",
    "receiver": "app/src/main/java/com/termux/app/BootstrapHandoffReceiver.java",
    "client": "app/src/main/java/com/termux/app/BootstrapLoaderClient.java",
    "gate": "app/src/main/java/com/termux/app/BootstrapGateActivity.java",
    "integrity": "app/src/main/java/com/termux/app/BootstrapIntegrityVerifier.java",
    "native": "app/src/main/cpp/termux-bootstrap.c",
    "app_gradle": "app/build.gradle",
    "loader_gradle": "app/loader/build.gradle",
}


class ValidationError(ValueError):
    pass


def load(root: Path = ROOT) -> dict[str, str]:
    result: dict[str, str] = {}
    for key, relative in FILES.items():
        path = root / relative
        try:
            result[key] = path.read_text(encoding="utf-8")
        except OSError as exc:
            raise ValidationError(f"{relative}: {exc}") from exc
    return result


def contains(text: str, *needles: str) -> bool:
    return all(needle in text for needle in needles)


def validate(files: Mapping[str, str]) -> list[str]:
    errors: list[str] = []
    missing = sorted(set(FILES) - set(files))
    if missing:
        return [f"missing logical files: {missing}"]

    loader_manifest = files["loader_manifest"]
    host_manifest = files["host_manifest"]
    contract = files["contract"]
    activity = files["activity"]
    service = files["service"]
    policy = files["policy"]
    provider = files["provider"]
    receiver = files["receiver"]
    client = files["client"]
    gate = files["gate"]
    integrity = files["integrity"]
    native = files["native"]
    app_gradle = files["app_gradle"]
    loader_gradle = files["loader_gradle"]

    permission = "com.termux.rafacodephi.permission.BOOTSTRAP_HANDOFF"
    if not contains(loader_manifest, f'android:permission="{permission}"',
                    'android:usesCleartextTraffic="false"',
                    'android:exported="false"',
                    'android:grantUriPermissions="true"',
                    'BOOTSTRAP_ACQUIRE_HANDOFF_CAPABLE'):
        errors.append("loader manifest does not enforce signature entry, private provider and cleartext block")
    if not contains(host_manifest,
                    '${TERMUX_PACKAGE_NAME}.permission.BOOTSTRAP_HANDOFF',
                    'android:protectionLevel="signature"',
                    'com.termux.app.BootstrapHandoffReceiver',
                    'com.termux.app.BootstrapGateActivity'):
        errors.append("host manifest does not declare signature custody and launcher gate")
    if 'android:targetActivity="com.termux.app.BootstrapGateActivity"' not in host_manifest:
        errors.append("HomeActivity does not target BootstrapGateActivity")

    if "EXTRA_TARGET_DIR" in contract or "target_dir" in contract:
        errors.append("contract exposes an arbitrary target directory")
    if not contains(contract, "HANDOFF_PERMISSION", "PROVIDER_AUTHORITY",
                    "ACTION_BOOTSTRAP_VERIFIED", "EXTRA_SHA256"):
        errors.append("contract is missing handoff custody identifiers")
    if "EXTRA_TARGET_DIR" in activity or "targetDir" in activity:
        errors.append("loader activity accepts a target directory")
    if not contains(activity, "BootstrapSourcePolicy.requireAbi",
                    "BootstrapSourcePolicy.requireSha256",
                    "BootstrapSourcePolicy.requireInitialUrl"):
        errors.append("loader activity does not validate all request pins")

    forbidden_extraction = ("ZipInputStream", "ZipEntry", "targetDir", "target_dir")
    if any(token in service for token in forbidden_extraction):
        errors.append("loader service still extracts or handles a host path")
    if not contains(service, "MAX_DOWNLOAD_BYTES", "setInstanceFollowRedirects(false)",
                    "requireSameOriginRedirect", "SHA256_MISMATCH", "getFD().sync()",
                    "grantUriPermission", "setPackage", "HANDOFF_PERMISSION",
                    "EXTRA_VERIFIED_BYTES", "EXTRA_SHA256"):
        errors.append("loader service lacks bounded download, SHA custody or explicit handoff")
    if "getContentLengthLong" in service:
        errors.append("loader service uses an API newer than the minimum SDK")

    if not contains(policy, '"https"', "MAX_DOWNLOAD_BYTES", "MAX_REDIRECTS",
                    "sameOrigin", "NON_STANDARD_HTTPS_PORT_BLOCKED",
                    "CROSS_ORIGIN_REDIRECT_BLOCKED"):
        errors.append("source policy does not enforce HTTPS same-origin bounded redirects")
    if not contains(provider, "MODE_READ_ONLY", "READ_ONLY_PROVIDER",
                    "SAFE_NAME", "getCanonicalPath"):
        errors.append("provider is not read-only or path bounded")

    if not contains(receiver, "MessageDigest.getInstance(\"SHA-256\")",
                    "BootstrapIntegrityVerifier.blake3Hex", "ZipFile",
                    "MAX_ZIP_ENTRIES", "MAX_UNCOMPRESSED_BYTES",
                    "MAX_COMPRESSION_RATIO", "SYMLINKS.txt", "Os.rename",
                    "claim_allowed", "revokeUriPermission"):
        errors.append("host receiver lacks double hash, ZIP budgets, atomic custody or receipt")
    if not contains(client, "checkSignatures", "SIGNATURE_MATCH",
                    "EXTERNAL_BOOTSTRAP_URL_AARCH64",
                    "EXTERNAL_BOOTSTRAP_SHA256_AARCH64",
                    "EXTERNAL_CANONICAL_BLAKE3_NOT_CONFIGURED",
                    "BOOTSTRAP_ACQUIRE_HANDOFF_CAPABLE"):
        errors.append("host client does not pin source, signature and loader contract")
    if not contains(gate, "requestIfConfigured", "TermuxActivity.class"):
        errors.append("launcher gate does not select loader before terminal")
    if not contains(integrity, "blake3Hex(@NonNull File", "maxBytes",
                    "FileInputStream"):
        errors.append("integrity verifier lacks bounded streaming BLAKE3")

    lowered_native = native.lower()
    if any(token in lowered_native for token in ("malloc(", "calloc(", "realloc(")):
        errors.append("native inbox reader allocates native heap")
    if not contains(native, "O_NOFOLLOW", "/proc/self/cmdline",
                    "MAX_EXTERNAL_BOOTSTRAP_BYTES", "errno == ENOENT",
                    "S_ISREG", "SetByteArrayRegion"):
        errors.append("native inbox reader lacks identity, no-follow, size or embedded fallback boundaries")

    external_envs = (
        "TERMUX_EXTERNAL_BOOTSTRAP_URL_AARCH64",
        "TERMUX_EXTERNAL_BOOTSTRAP_URL_ARM",
        "TERMUX_EXTERNAL_BOOTSTRAP_URL_I686",
        "TERMUX_EXTERNAL_BOOTSTRAP_URL_X86_64",
        "TERMUX_EXTERNAL_BOOTSTRAP_SHA256_AARCH64",
        "TERMUX_EXTERNAL_BOOTSTRAP_SHA256_ARM",
        "TERMUX_EXTERNAL_BOOTSTRAP_SHA256_I686",
        "TERMUX_EXTERNAL_BOOTSTRAP_SHA256_X86_64",
    )
    if not all(env in app_gradle for env in external_envs):
        errors.append("app build does not expose all optional external pins")
    for env in external_envs:
        pattern = re.compile(rf'System\.getenv\("{re.escape(env)}"\)\s*\?:\s*""')
        if pattern.search(app_gradle) is None:
            errors.append(f"{env} must default to an empty TOKEN_VAZIO-equivalent string")
    if not contains(app_gradle, "validateExternalBootstrapPin", "https",
                    "EXTERNAL_BOOTSTRAP_URL_AARCH64"):
        errors.append("app build does not fail closed on partial or non-HTTPS pins")
    signing_vars = (
        "TERMUX_ENABLE_RELEASE_SIGNING", "TERMUX_RELEASE_KEYSTORE_FILE",
        "TERMUX_RELEASE_KEYSTORE_PASSWORD", "TERMUX_RELEASE_KEY_ALIAS",
        "TERMUX_RELEASE_KEY_PASSWORD",
    )
    if not all(value in loader_gradle for value in signing_vars):
        errors.append("loader release signing is not tied to the host signing inputs")

    return errors


def report(errors: list[str]) -> dict[str, object]:
    return {
        "schema": "termux.rafacodephi.loader_secure_handoff_validation.v1",
        "status": "PASS" if not errors else "FAIL",
        "implementation_state": "IMPLEMENTED_SECURITY_GATED" if not errors else "BLOCKED",
        "claim_allowed": False,
        "security_boundaries": {
            "signature_permission": not errors,
            "arbitrary_target_removed": not errors,
            "https_same_origin": not errors,
            "download_budget": not errors,
            "read_only_uri": not errors,
            "host_sha256_and_blake3": not errors,
            "bounded_zip_structure": not errors,
            "native_malloc": False,
            "embedded_fallback_only_when_external_absent": not errors,
        },
        "remaining_evidence": [
            "Gradle host and loader build artifacts",
            "matching signer certificate receipt",
            "instrumented unauthorized-caller and URI-grant tests",
            "exact URL/SHA-256/BLAKE3 build pins when external route is enabled",
            "DEVICE_RECEIPT_COMPLETE on target Android",
        ],
        "errors": errors,
    }


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--root", type=Path, default=ROOT)
    parser.add_argument("--write-report", type=Path)
    args = parser.parse_args(argv)
    try:
        errors = validate(load(args.root))
    except ValidationError as exc:
        errors = [str(exc)]
    result = report(errors)
    text = json.dumps(result, ensure_ascii=False, sort_keys=True, indent=2) + "\n"
    if args.write_report:
        args.write_report.parent.mkdir(parents=True, exist_ok=True)
        args.write_report.write_text(text, encoding="utf-8")
    print(text, end="")
    return 0 if not errors else 1


if __name__ == "__main__":
    raise SystemExit(main())
