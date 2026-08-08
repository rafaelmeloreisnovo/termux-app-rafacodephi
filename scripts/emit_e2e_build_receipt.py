#!/usr/bin/env python3
"""Emit a build receipt binding a termux-packages bootstrap manifest to an APK.

The receipt proves file identity at build time. It does not prove installation or
physical runtime; those remain TOKEN_VAZIO until device receipts are collected.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import subprocess
import tempfile
from pathlib import Path

APP_REPO = "rafaelmeloreisnovo/termux-app-rafacodephi"
SOURCE_REPO = "rafaelmeloreisnovo/termux-packages"
SOURCE_SCHEMA = "rafcodephi.bootstrap-source-manifest/v1"
BUILD_SCHEMA = "rafcodephi.e2e-build-receipt/v1"
PACKAGE = "com.termux.rafacodephi"
PREFIX = f"/data/data/{PACKAGE}/files/usr"
ABIS = {"armeabi-v7a", "arm64-v8a"}
HEX = set("0123456789abcdef")


class BuildReceiptError(RuntimeError):
    pass


def sha256_file(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as fh:
        for chunk in iter(lambda: fh.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()


def is_hex(value: object, size: int) -> bool:
    return isinstance(value, str) and len(value) == size and all(c in HEX for c in value)


def load_json(path: Path) -> dict:
    try:
        doc = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        raise BuildReceiptError(f"cannot read JSON {path}: {exc}") from exc
    if not isinstance(doc, dict):
        raise BuildReceiptError(f"{path} must contain a JSON object")
    return doc


def resolve_git_commit(explicit: str | None) -> str:
    value = explicit
    if not value:
        try:
            value = subprocess.check_output(
                ["git", "rev-parse", "HEAD"], text=True, stderr=subprocess.DEVNULL
            ).strip()
        except (OSError, subprocess.CalledProcessError) as exc:
            raise BuildReceiptError("--git-commit required outside a Git checkout") from exc
    if not is_hex(value, 40):
        raise BuildReceiptError("app git commit must be lowercase 40-hex")
    return value


def validate_source_manifest(doc: dict) -> None:
    expected = {
        "schema": SOURCE_SCHEMA,
        "repository": SOURCE_REPO,
        "package": PACKAGE,
        "prefix": PREFIX,
        "profile": "real-pkg",
        "artifact_gate": "PASS",
        "device_runtime": "TOKEN_VAZIO",
        "claim_allowed": False,
    }
    drift = [k for k, v in expected.items() if doc.get(k) != v]
    if drift:
        raise BuildReceiptError("bootstrap source manifest drift: " + ", ".join(drift))
    if doc.get("android_abi") not in ABIS:
        raise BuildReceiptError("bootstrap source android_abi invalid")
    if not is_hex(doc.get("git_commit"), 40):
        raise BuildReceiptError("bootstrap source git_commit invalid")
    for key in ("artifact_sha256", "bootstrap_profile_sha256"):
        if not is_hex(doc.get(key), 64):
            raise BuildReceiptError(f"bootstrap source {key} invalid")


def emit(*, source_manifest_path: Path, bootstrap_zip: Path, apk: Path, git_commit: str) -> dict:
    source = load_json(source_manifest_path)
    validate_source_manifest(source)
    if not bootstrap_zip.is_file():
        raise BuildReceiptError(f"bootstrap ZIP missing: {bootstrap_zip}")
    if not apk.is_file():
        raise BuildReceiptError(f"APK missing: {apk}")

    bootstrap_sha = sha256_file(bootstrap_zip)
    if bootstrap_sha != source["artifact_sha256"]:
        raise BuildReceiptError(
            f"bootstrap hash mismatch: source={source['artifact_sha256']} local={bootstrap_sha}"
        )

    return {
        "schema": BUILD_SCHEMA,
        "repository": APP_REPO,
        "git_commit": git_commit,
        "apk_name": apk.name,
        "apk_sha256": sha256_file(apk),
        "android_abi": source["android_abi"],
        "bootstrap": {
            "source_repository": source["repository"],
            "source_git_commit": source["git_commit"],
            "source_manifest_sha256": sha256_file(source_manifest_path),
            "artifact_sha256": bootstrap_sha,
            "profile_sha256": source["bootstrap_profile_sha256"],
        },
        "build_gate": "PASS",
        "device_runtime": "TOKEN_VAZIO",
        "claim_allowed": False,
    }


def self_test() -> int:
    with tempfile.TemporaryDirectory() as td:
        root = Path(td)
        bootstrap = root / "bootstrap.zip"
        bootstrap.write_bytes(b"bootstrap-real-pkg-fixture")
        apk = root / "app.apk"
        apk.write_bytes(b"apk-fixture")
        source = {
            "schema": SOURCE_SCHEMA,
            "repository": SOURCE_REPO,
            "git_commit": "a" * 40,
            "artifact_name": bootstrap.name,
            "artifact_size": bootstrap.stat().st_size,
            "artifact_sha256": sha256_file(bootstrap),
            "bootstrap_profile_sha256": "b" * 64,
            "arch": "aarch64",
            "android_abi": "arm64-v8a",
            "package": PACKAGE,
            "prefix": PREFIX,
            "profile": "real-pkg",
            "artifact_gate": "PASS",
            "device_runtime": "TOKEN_VAZIO",
            "claim_allowed": False,
        }
        source_path = root / "source.json"
        source_path.write_text(json.dumps(source), encoding="utf-8")
        out = emit(
            source_manifest_path=source_path,
            bootstrap_zip=bootstrap,
            apk=apk,
            git_commit="c" * 40,
        )
        assert out["build_gate"] == "PASS"
        assert out["bootstrap"]["artifact_sha256"] == source["artifact_sha256"]
        assert out["claim_allowed"] is False

        bootstrap.write_bytes(b"drift")
        try:
            emit(
                source_manifest_path=source_path,
                bootstrap_zip=bootstrap,
                apk=apk,
                git_commit="c" * 40,
            )
        except BuildReceiptError:
            pass
        else:
            raise AssertionError("bootstrap drift must be rejected")
    print("SELF_TEST PASS: source→bootstrap→APK binding=PASS drift=BLOCKED device=TOKEN_VAZIO")
    return 0


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--bootstrap-source-manifest")
    ap.add_argument("--bootstrap-zip")
    ap.add_argument("--apk")
    ap.add_argument("--git-commit")
    ap.add_argument("--out")
    ap.add_argument("--self-test", action="store_true")
    args = ap.parse_args()

    if args.self_test:
        return self_test()
    if not args.bootstrap_source_manifest or not args.bootstrap_zip or not args.apk or not args.out:
        ap.error("--bootstrap-source-manifest, --bootstrap-zip, --apk and --out are required")

    try:
        doc = emit(
            source_manifest_path=Path(args.bootstrap_source_manifest),
            bootstrap_zip=Path(args.bootstrap_zip),
            apk=Path(args.apk),
            git_commit=resolve_git_commit(args.git_commit),
        )
    except BuildReceiptError as exc:
        print(f"BLOCKED: {exc}")
        return 2

    out = Path(args.out)
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text(json.dumps(doc, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(f"E2E_BUILD_RECEIPT=PASS path={out} sha256={sha256_file(out)}")
    print("DEVICE_RUNTIME=TOKEN_VAZIO")
    print("CLAIM_ALLOWED=false")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
