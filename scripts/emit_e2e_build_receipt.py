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
import zipfile
from pathlib import Path

import verify_apk_bootstrap_payload

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


def _unique_keys(pairs):
    out = {}
    for key, value in pairs:
        if key in out:
            raise BuildReceiptError(f"duplicate JSON key: {key}")
        out[key] = value
    return out


def load_json(path: Path) -> dict:
    try:
        if path.stat().st_size > (1 << 20):
            raise BuildReceiptError("source manifest exceeds 1 MiB")
        doc = json.loads(path.read_text(encoding="utf-8"), object_pairs_hook=_unique_keys)
    except (OSError, UnicodeError, json.JSONDecodeError) as exc:
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
    arch_abi = {"arm": "armeabi-v7a", "aarch64": "arm64-v8a"}
    if not isinstance(doc.get("arch"), str) or doc["arch"] not in arch_abi:
        raise BuildReceiptError("bootstrap source arch invalid")
    if doc.get("android_abi") != arch_abi[doc["arch"]]:
        raise BuildReceiptError("bootstrap source architecture/ABI drift")
    if type(doc.get("artifact_size")) is not int or doc["artifact_size"] <= 0:
        raise BuildReceiptError("bootstrap source artifact_size invalid")
    if not is_hex(doc.get("git_commit"), 40):
        raise BuildReceiptError("bootstrap source git_commit invalid")
    for key in ("artifact_sha256", "bootstrap_profile_sha256"):
        if not is_hex(doc.get(key), 64):
            raise BuildReceiptError(f"bootstrap source {key} invalid")


def verify_real_bootstrap_in_apk(apk: Path, bootstrap_zip: Path, source: dict) -> None:
    """Exact ZIP-byte binding to APK's native ELF, not just unrelated SHA values.

    Reuses the repository's existing ELF bootstrap parser; this proves a
    static payload relationship and never claims installation or runtime.
    """
    abi = source["android_abi"]
    try:
        if bootstrap_zip.stat().st_size != source["artifact_size"]:
            raise BuildReceiptError("BOOTSTRAP_ARTIFACT_SIZE_DRIFT")
        if bootstrap_zip.stat().st_size > (128 << 20):
            raise BuildReceiptError("BOOTSTRAP_ARTIFACT_TOO_LARGE")
        with zipfile.ZipFile(bootstrap_zip) as boot:
            if len(boot.namelist()) != len(set(boot.namelist())):
                raise BuildReceiptError("BOOTSTRAP_DUPLICATE_ZIP_ENTRY")
            info = boot.getinfo("BOOTSTRAP_PROFILE.json")
            if info.file_size > 65536:
                raise BuildReceiptError("BOOTSTRAP_PROFILE_TOO_LARGE")
            profile = json.loads(boot.read(info), object_pairs_hook=_unique_keys)
            if not isinstance(profile, dict):
                raise BuildReceiptError("BOOTSTRAP_PROFILE_NOT_OBJECT")
            expected_fields = {
                "schema": "rafcodephi-bootstrap-profile/v1",
                "profile": "real-pkg",
                "package_layer": "real-pkg",
                "package_name": PACKAGE,
                "prefix": PREFIX,
                "arch": source["arch"],
                "device_validation": "TOKEN_VAZIO",
                "claim_allowed": False,
                "release_allowed": False,
            }
            if any(profile.get(k) != v for k, v in expected_fields.items()):
                raise BuildReceiptError("BOOTSTRAP_PROFILE_DRIFT")
            raw_profile = json.dumps(profile, sort_keys=True, separators=(",", ":"), ensure_ascii=False).encode()
            if hashlib.sha256(raw_profile).hexdigest() != source["bootstrap_profile_sha256"]:
                raise BuildReceiptError("BOOTSTRAP_PROFILE_SHA256_DRIFT")
        member = f"lib/{abi}/libtermux-bootstrap.so"
        with zipfile.ZipFile(apk) as package:
            matches = [x for x in package.infolist() if x.filename == member]
            if len(matches) != 1:
                raise BuildReceiptError("APK_MISSING_OR_DUPLICATE_NATIVE_BOOTSTRAP")
            if matches[0].file_size > (256 << 20):
                raise BuildReceiptError("APK_NATIVE_BOOTSTRAP_TOO_LARGE")
            native = package.read(matches[0])
        embedded = verify_apk_bootstrap_payload.embedded_zip(native, abi)
        if embedded != bootstrap_zip.read_bytes():
            raise BuildReceiptError("APK_EMBEDDED_BOOTSTRAP_MISMATCH")
    except (OSError, zipfile.BadZipFile, zipfile.LargeZipFile, KeyError,
            ValueError, TypeError, UnicodeError) as exc:
        if isinstance(exc, BuildReceiptError):
            raise
        raise BuildReceiptError("APK_BOOTSTRAP_BINDING_BLOCKED: " + str(exc)[:200]) from exc


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
    verify_real_bootstrap_in_apk(apk, bootstrap_zip, source)

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
    from unittest.mock import patch

    with tempfile.TemporaryDirectory() as td:
        root = Path(td)
        bootstrap = root / "bootstrap.zip"
        apk = root / "app.apk"
        profile = {
            "schema": "rafcodephi-bootstrap-profile/v1",
            "profile": "real-pkg", "package_layer": "real-pkg",
            "package_name": PACKAGE, "prefix": PREFIX, "arch": "aarch64",
            "device_validation": "TOKEN_VAZIO", "claim_allowed": False,
            "release_allowed": False,
        }
        with zipfile.ZipFile(bootstrap, "w") as archive:
            archive.writestr("BOOTSTRAP_PROFILE.json", json.dumps(profile))
        with zipfile.ZipFile(apk, "w") as archive:
            archive.writestr("lib/arm64-v8a/libtermux-bootstrap.so", b"fixture-native")
        profile_bytes = json.dumps(profile, sort_keys=True, separators=(",", ":"), ensure_ascii=False).encode()
        source = {
            "schema": SOURCE_SCHEMA,
            "repository": SOURCE_REPO, "git_commit": "a" * 40,
            "artifact_name": bootstrap.name, "artifact_size": bootstrap.stat().st_size,
            "artifact_sha256": sha256_file(bootstrap),
            "bootstrap_profile_sha256": hashlib.sha256(profile_bytes).hexdigest(),
            "arch": "aarch64", "android_abi": "arm64-v8a",
            "package": PACKAGE, "prefix": PREFIX,
            "profile": "real-pkg", "artifact_gate": "PASS",
            "device_runtime": "TOKEN_VAZIO", "claim_allowed": False,
        }
        source_path = root / "source.json"
        source_path.write_text(json.dumps(source), encoding="utf-8")
        with patch.object(verify_apk_bootstrap_payload, "embedded_zip",
                          return_value=bootstrap.read_bytes()):
            out = emit(source_manifest_path=source_path, bootstrap_zip=bootstrap,
                       apk=apk, git_commit="c" * 40)
        assert out["build_gate"] == "PASS"
        assert out["bootstrap"]["artifact_sha256"] == source["artifact_sha256"]
        assert out["claim_allowed"] is False

        with patch.object(verify_apk_bootstrap_payload, "embedded_zip",
                          return_value=b"unrelated-bootstrap"):
            try:
                emit(source_manifest_path=source_path, bootstrap_zip=bootstrap,
                     apk=apk, git_commit="c" * 40)
            except BuildReceiptError as exc:
                assert "APK_EMBEDDED_BOOTSTRAP_MISMATCH" in str(exc)
            else:
                raise AssertionError("unrelated embedded bootstrap must be rejected")
        bootstrap.write_bytes(b"drift")
        try:
            emit(source_manifest_path=source_path, bootstrap_zip=bootstrap,
                 apk=apk, git_commit="c" * 40)
        except BuildReceiptError:
            pass
        else:
            raise AssertionError("bootstrap hash drift must be rejected")
    print("SELF_TEST PASS: source->bootstrap->APK identity=PASS drift=BLOCKED unrelated_payload=BLOCKED")
    print("DEVICE_RUNTIME=TOKEN_VAZIO CLAIM_ALLOWED=false")
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
