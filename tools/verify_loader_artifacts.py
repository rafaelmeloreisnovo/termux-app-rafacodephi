#!/usr/bin/env python3
"""Verify functional loader APK structure and host/loader signer equality."""
from __future__ import annotations

import argparse
import hashlib
import json
import re
import shutil
import subprocess
import zipfile
from pathlib import Path

REQUIRED_CLASSES = (
    b"Lcom/termux/rafacodephi/loader/LoaderActivity;",
    b"Lcom/termux/rafacodephi/loader/BootstrapInstallService;",
    b"Lcom/termux/rafacodephi/loader/BootstrapSourcePolicy;",
    b"Lcom/termux/rafacodephi/loader/VerifiedBootstrapProvider;",
)
REQUIRED_MANIFEST = (
    "BOOTSTRAP_ACQUIRE_HANDOFF_CAPABLE",
    "com.termux.rafacodephi.permission.BOOTSTRAP_HANDOFF",
    "com.termux.rafacodephi.loader.LoaderActivity",
    "com.termux.rafacodephi.loader.BootstrapInstallService",
    "com.termux.rafacodephi.loader.VerifiedBootstrapProvider",
    "com.termux.rafacodephi.loader.bootstrap",
)


def run(command: list[str]) -> str:
    result = subprocess.run(command, check=True, text=True, capture_output=True)
    return result.stdout + result.stderr


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(65536), b""):
            digest.update(chunk)
    return digest.hexdigest()


def find_tool(name: str) -> str:
    direct = shutil.which(name)
    if direct:
        return direct
    root = Path(__file__).resolve().parents[1]
    script = root / "scripts" / "find_android_sdk_tool.sh"
    return run(["bash", str(script), name]).strip()


def manifest_text(apk: Path) -> str:
    apkanalyzer = find_tool("apkanalyzer")
    return run([apkanalyzer, "manifest", "print", str(apk)])


def certificate(apk: Path) -> str:
    apksigner = find_tool("apksigner")
    output = run([apksigner, "verify", "--print-certs", str(apk)])
    match = re.search(r"Signer #1 certificate SHA-256 digest: ([0-9a-fA-F:]+)", output)
    if match is None:
        raise ValueError("certificate SHA-256 not found")
    return match.group(1).replace(":", "").lower()


def inspect(loader: Path, host: Path) -> dict[str, object]:
    errors: list[str] = []
    manifest = manifest_text(loader)
    if "com.termux.rafacodephi.loader" not in manifest:
        errors.append("loader package missing from manifest")
    for required in REQUIRED_MANIFEST:
        if required not in manifest:
            errors.append(f"manifest missing {required}")

    with zipfile.ZipFile(loader) as archive:
        names = archive.namelist()
        dex_names = [name for name in names if re.fullmatch(r"classes\d*\.dex", name)]
        if dex_names != ["classes.dex"]:
            errors.append(f"expected one classes.dex, observed {dex_names}")
        else:
            dex = archive.read("classes.dex")
            for descriptor in REQUIRED_CLASSES:
                if descriptor not in dex:
                    errors.append(f"DEX missing {descriptor.decode()}")
        embedded = [name for name in names if re.search(
            r"(^|/)(bootstrap|payload).*\.(zip|tar|gz)$", name, re.IGNORECASE)]
        if embedded:
            errors.append(f"loader embeds bootstrap payloads: {embedded}")

    loader_cert = certificate(loader)
    host_cert = certificate(host)
    if not loader_cert or loader_cert != host_cert:
        errors.append("host and loader signer certificates differ")

    return {
        "schema": "termux.rafacodephi.loader_artifact_verification.v1",
        "status": "PASS" if not errors else "FAIL",
        "loader_sha256": sha256(loader),
        "host_sha256": sha256(host),
        "loader_certificate_sha256": loader_cert,
        "host_certificate_sha256": host_cert,
        "matching_certificate": loader_cert == host_cert,
        "claim_allowed": False,
        "errors": errors,
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--loader", type=Path, required=True)
    parser.add_argument("--host", type=Path, required=True)
    parser.add_argument("--write-report", type=Path)
    args = parser.parse_args()
    try:
        result = inspect(args.loader, args.host)
    except Exception as error:
        result = {
            "schema": "termux.rafacodephi.loader_artifact_verification.v1",
            "status": "FAIL",
            "claim_allowed": False,
            "errors": [str(error)],
        }
    text = json.dumps(result, ensure_ascii=False, sort_keys=True, indent=2) + "\n"
    if args.write_report:
        args.write_report.parent.mkdir(parents=True, exist_ok=True)
        args.write_report.write_text(text, encoding="utf-8")
    print(text, end="")
    return 0 if result["status"] == "PASS" else 1


if __name__ == "__main__":
    raise SystemExit(main())
