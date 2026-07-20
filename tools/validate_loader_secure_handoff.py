#!/usr/bin/env python3
"""Fail-closed static validator for the loader/host bootstrap handoff v2."""
from __future__ import annotations

import argparse
import json
import re
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
    "pin_gradle": "gradle/bootstrap-external.gradle",
    "root_gradle": "build.gradle",
    "loader_gradle": "app/loader/build.gradle",
}

class ValidationError(ValueError): pass

def load(root: Path = ROOT) -> dict[str, str]:
    result = {}
    for key, relative in FILES.items():
        try: result[key] = (root / relative).read_text(encoding="utf-8")
        except OSError as exc: raise ValidationError(f"{relative}: {exc}") from exc
    return result

def all_in(text: str, *needles: str) -> bool:
    return all(item in text for item in needles)

def validate(files: Mapping[str, str]) -> list[str]:
    errors: list[str] = []
    if set(FILES) - set(files): return ["missing logical files"]
    lm, hm = files["loader_manifest"], files["host_manifest"]
    contract, activity, service = files["contract"], files["activity"], files["service"]
    policy, provider = files["policy"], files["provider"]
    receiver, client, gate = files["receiver"], files["client"], files["gate"]
    integrity, native = files["integrity"], files["native"]
    pins, root_gradle, loader_gradle = files["pin_gradle"], files["root_gradle"], files["loader_gradle"]

    permission = "com.termux.rafacodephi.permission.BOOTSTRAP_HANDOFF"
    if not all_in(lm, f'android:permission="{permission}"', 'android:usesCleartextTraffic="false"',
                  'android:grantUriPermissions="true"', 'BOOTSTRAP_ACQUIRE_HANDOFF_CAPABLE'):
        errors.append("loader manifest security boundary incomplete")
    if not all_in(hm, '${TERMUX_PACKAGE_NAME}.permission.BOOTSTRAP_HANDOFF',
                  'android:protectionLevel="signature"', 'BootstrapHandoffReceiver',
                  'BootstrapGateActivity', 'android:targetActivity="com.termux.app.BootstrapGateActivity"'):
        errors.append("host manifest signature custody incomplete")
    if "EXTRA_TARGET_DIR" in contract or "target_dir" in contract or "targetDir" in activity:
        errors.append("arbitrary target directory exposed")
    if not all_in(contract, "HANDOFF_PERMISSION", "PROVIDER_AUTHORITY", "ACTION_BOOTSTRAP_VERIFIED"):
        errors.append("contract custody identifiers missing")
    if not all_in(activity, "requireAbi", "requireSha256", "requireInitialUrl"):
        errors.append("loader request validation incomplete")
    if any(token in service for token in ("ZipInputStream", "ZipEntry", "targetDir", "target_dir")):
        errors.append("loader still extracts or handles host path")
    if not all_in(service, "MAX_DOWNLOAD_BYTES", "setInstanceFollowRedirects(false)",
                  "requireSameOriginRedirect", "SHA256_MISMATCH", "getFD().sync()",
                  "grantUriPermission", "setPackage", "HANDOFF_PERMISSION", "EXTRA_VERIFIED_BYTES"):
        errors.append("bounded acquisition or explicit handoff incomplete")
    if "getContentLengthLong" in service: errors.append("minimum SDK incompatible API")
    if not all_in(policy, '"https"', "MAX_REDIRECTS", "sameOrigin",
                  "NON_STANDARD_HTTPS_PORT_BLOCKED", "CROSS_ORIGIN_REDIRECT_BLOCKED"):
        errors.append("HTTPS same-origin policy incomplete")
    if not all_in(provider, "MODE_READ_ONLY", "READ_ONLY_PROVIDER", "SAFE_NAME", "getCanonicalPath"):
        errors.append("read-only provider boundary incomplete")
    if not all_in(receiver, 'MessageDigest.getInstance("SHA-256")', "blake3Hex", "ZipFile",
                  "MAX_ZIP_ENTRIES", "MAX_UNCOMPRESSED_BYTES", "MAX_COMPRESSION_RATIO",
                  "SYMLINKS.txt", "Os.rename", "claim_allowed", "revokeUriPermission"):
        errors.append("host double-hash ZIP custody incomplete")
    if not all_in(client, "checkSignatures", "SIGNATURE_MATCH", "EXTERNAL_BOOTSTRAP_URL_AARCH64",
                  "EXTERNAL_BOOTSTRAP_SHA256_AARCH64", "EXTERNAL_CANONICAL_BLAKE3_NOT_CONFIGURED",
                  "BOOTSTRAP_ACQUIRE_HANDOFF_CAPABLE"):
        errors.append("host pin/signature client incomplete")
    if not all_in(gate, "requestIfConfigured", "TermuxActivity.class"):
        errors.append("launcher gate incomplete")
    if not all_in(integrity, "blake3Hex(@NonNull File", "maxBytes", "FileInputStream"):
        errors.append("streaming BLAKE3 missing")
    lower_native = native.lower()
    if any(token in lower_native for token in ("malloc(", "calloc(", "realloc("))):
        errors.append("native heap allocation introduced")
    if not all_in(native, "O_NOFOLLOW", "/proc/self/cmdline", "MAX_EXTERNAL_BOOTSTRAP_BYTES",
                  "errno == ENOENT", "S_ISREG", "SetByteArrayRegion"):
        errors.append("native inbox boundary incomplete")

    envs = (
        "TERMUX_EXTERNAL_BOOTSTRAP_URL_AARCH64", "TERMUX_EXTERNAL_BOOTSTRAP_URL_ARM",
        "TERMUX_EXTERNAL_BOOTSTRAP_URL_I686", "TERMUX_EXTERNAL_BOOTSTRAP_URL_X86_64",
        "TERMUX_EXTERNAL_BOOTSTRAP_SHA256_AARCH64", "TERMUX_EXTERNAL_BOOTSTRAP_SHA256_ARM",
        "TERMUX_EXTERNAL_BOOTSTRAP_SHA256_I686", "TERMUX_EXTERNAL_BOOTSTRAP_SHA256_X86_64",
    )
    for env in envs:
        if env not in pins: errors.append(f"pin missing: {env}")
        if re.search(rf'System\.getenv\("{re.escape(env)}"\)\s*\?:\s*""', pins) is None:
            errors.append(f"{env} must default empty")
    if not all_in(pins, "validateBootstrapPin", "https", "buildConfigField"):
        errors.append("Gradle pin validation incomplete")
    if 'apply from: "$rootDir/gradle/bootstrap-external.gradle"' not in root_gradle:
        errors.append("root Gradle does not apply external pin contract")
    signing = ("TERMUX_ENABLE_RELEASE_SIGNING", "TERMUX_RELEASE_KEYSTORE_FILE",
               "TERMUX_RELEASE_KEYSTORE_PASSWORD", "TERMUX_RELEASE_KEY_ALIAS",
               "TERMUX_RELEASE_KEY_PASSWORD")
    if not all(item in loader_gradle for item in signing):
        errors.append("loader signer boundary not shared with host")
    return errors

def report(errors: list[str]) -> dict[str, object]:
    return {
        "schema": "termux.rafacodephi.loader_secure_handoff_validation.v1",
        "status": "PASS" if not errors else "FAIL",
        "implementation_state": "IMPLEMENTED_SECURITY_GATED" if not errors else "BLOCKED",
        "claim_allowed": False,
        "remaining_evidence": [
            "host and loader Gradle artifacts", "matching signer receipt",
            "instrumented unauthorized-caller and URI-grant tests",
            "exact URL/SHA-256/BLAKE3 pins when enabled", "DEVICE_RECEIPT_COMPLETE",
        ],
        "errors": errors,
    }

def main(argv=None) -> int:
    parser = argparse.ArgumentParser(); parser.add_argument("--root", type=Path, default=ROOT)
    parser.add_argument("--write-report", type=Path); args = parser.parse_args(argv)
    try: errors = validate(load(args.root))
    except ValidationError as exc: errors = [str(exc)]
    result = report(errors); text = json.dumps(result, ensure_ascii=False, sort_keys=True, indent=2) + "\n"
    if args.write_report:
        args.write_report.parent.mkdir(parents=True, exist_ok=True); args.write_report.write_text(text, encoding="utf-8")
    print(text, end=""); return 0 if not errors else 1

if __name__ == "__main__": raise SystemExit(main())
