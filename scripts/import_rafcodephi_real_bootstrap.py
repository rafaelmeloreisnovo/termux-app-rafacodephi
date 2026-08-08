#!/usr/bin/env python3
"""Import a source-built RAFCODEPHI bootstrap into the exact embedded ARM slot.

This importer is intentionally stricter than a normal ZIP copy. It refuses bridge
payloads, wrong package/prefix/ABI, malformed provenance, non-ELF package-manager
backends and legacy com.termux critical-path references. Canonical Termux symlinks
from SYMLINKS.txt count as installed entries without being materialized prematurely.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
import shutil
import struct
import tempfile
import zipfile
from pathlib import Path

PACKAGE = "com.termux.rafacodephi"
PREFIX = f"/data/data/{PACKAGE}/files/usr"
LEGACY_PREFIX = b"/data/data/com.termux/files/usr"
PROFILE_SCHEMA = "rafcodephi-bootstrap-profile/v1"
MANIFEST_SCHEMA = "rafcodephi.real-bootstrap-sourcebuild/v1"
BRIDGE_MARKERS = (
    b"RAFCODEPHI pkg bridge",
    b"real apt backend is not installed yet",
    b"real apt/apt-get backend is not installed yet",
    b"real proot native binary is not installed yet",
)
REQUIRED = (
    "SYMLINKS.txt",
    "BOOTSTRAP_PROFILE.json",
    "bin/sh",
    "bin/pkg",
    "bin/apt",
    "bin/apt-get",
    "bin/dpkg",
    "bin/bash",
    "bin/busybox",
    "bin/proot",
    "etc/apt/sources.list",
)
ELF_REQUIRED = ("bin/apt", "bin/apt-get", "bin/dpkg", "bin/bash", "bin/busybox", "bin/proot")


def parse_manifest(path: Path) -> dict[str, str]:
    out: dict[str, str] = {}
    for raw in path.read_text(encoding="utf-8").splitlines():
        line = raw.strip()
        if not line or line.startswith("#"):
            continue
        if "=" not in line:
            raise SystemExit(f"manifest line is not key=value: {line!r}")
        key, value = line.split("=", 1)
        if not key or key in out:
            raise SystemExit(f"manifest key invalid or duplicated: {key!r}")
        out[key] = value
    return out


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def safe_zip_names(zf: zipfile.ZipFile) -> set[str]:
    names: set[str] = set()
    for info in zf.infolist():
        name = info.filename
        if not name or name.startswith("/") or "../" in name or name == ".." or "\\" in name:
            raise SystemExit(f"unsafe bootstrap ZIP entry: {name!r}")
        if name in names:
            raise SystemExit(f"duplicate bootstrap ZIP entry: {name}")
        names.add(name)
    return names


def parse_symlink_destinations(zf: zipfile.ZipFile, names: set[str]) -> set[str]:
    if "SYMLINKS.txt" not in names:
        raise SystemExit("SYMLINKS.txt missing")
    raw = zf.read("SYMLINKS.txt")
    if len(raw) > 1024 * 1024:
        raise SystemExit("SYMLINKS.txt exceeds 1 MiB")
    text = raw.decode("utf-8")
    destinations: set[str] = set()
    for number, line in enumerate(text.splitlines(), 1):
        if not line:
            continue
        parts = line.split("←")
        if len(parts) != 2 or not parts[0] or not parts[1]:
            raise SystemExit(f"malformed SYMLINKS.txt line {number}: {line!r}")
        target, link = parts
        if link.startswith("/") or ".." in link or "\\" in link:
            raise SystemExit(f"unsafe symlink destination line {number}: {link!r}")
        if link in destinations or link in names:
            raise SystemExit(f"duplicate/conflicting symlink destination: {link}")
        if target.startswith("/") and LEGACY_PREFIX.decode() in target:
            raise SystemExit(f"symlink target embeds forbidden legacy prefix: {target}")
        destinations.add(link)
    if not destinations:
        raise SystemExit("SYMLINKS.txt contains no symlinks")
    return destinations


def require_arm_elf(payload: bytes, label: str) -> None:
    if len(payload) < 20 or payload[:4] != b"\x7fELF":
        raise SystemExit(f"{label} is not ELF")
    if payload[4] != 1:
        raise SystemExit(f"{label} is not ELF32 (class={payload[4]})")
    if payload[5] != 1:
        raise SystemExit(f"{label} is not little-endian ELF")
    machine = struct.unpack_from("<H", payload, 18)[0]
    if machine != 40:
        raise SystemExit(f"{label} e_machine={machine}, expected EM_ARM=40")
    if LEGACY_PREFIX in payload:
        raise SystemExit(f"{label} embeds forbidden legacy prefix")


def validate(zip_path: Path, manifest_path: Path) -> dict[str, object]:
    manifest = parse_manifest(manifest_path)
    expected_manifest = {
        "schema": MANIFEST_SCHEMA,
        "package_name": PACKAGE,
        "prefix": PREFIX,
        "bridge_allowed": "false",
        "legacy_prefix_allowed": "false",
        "claim_allowed_device_runtime": "false",
        "device_runtime_proof": "TOKEN_VAZIO",
    }
    for key, expected in expected_manifest.items():
        if manifest.get(key) != expected:
            raise SystemExit(f"manifest contract mismatch {key}: {manifest.get(key)!r} != {expected!r}")

    declared = manifest.get("sha256_arm", "")
    actual = sha256(zip_path)
    if len(declared) != 64 or declared.lower() != actual:
        raise SystemExit(f"bootstrap SHA256 mismatch declared={declared!r} actual={actual}")

    with zipfile.ZipFile(zip_path, "r") as zf:
        if zf.testzip() is not None:
            raise SystemExit("bootstrap ZIP CRC validation failed")
        names = safe_zip_names(zf)
        symlink_destinations = parse_symlink_destinations(zf, names)
        available = names | symlink_destinations
        missing = [name for name in REQUIRED if name not in available]
        if missing:
            raise SystemExit("bootstrap installed entries missing: " + ",".join(missing))

        profile_raw = zf.read("BOOTSTRAP_PROFILE.json")
        if len(profile_raw) > 64 * 1024:
            raise SystemExit("BOOTSTRAP_PROFILE.json exceeds 64 KiB")
        profile = json.loads(profile_raw.decode("utf-8"))
        expected_profile = {
            "schema": PROFILE_SCHEMA,
            "profile": "real-pkg",
            "package_layer": "real-pkg",
            "package_name": PACKAGE,
            "prefix": PREFIX,
            "arch": "arm",
            "claim_allowed": False,
            "release_allowed": False,
            "device_validation": "TOKEN_VAZIO",
            "real_pkg_relocation_claim_allowed": False,
        }
        for key, expected in expected_profile.items():
            if profile.get(key) != expected:
                raise SystemExit(f"profile contract mismatch {key}: {profile.get(key)!r} != {expected!r}")
        required_profile = profile.get("required_entries")
        if not isinstance(required_profile, list) or not required_profile:
            raise SystemExit("profile required_entries missing/empty")
        for name in required_profile:
            if not isinstance(name, str) or name not in available:
                raise SystemExit(f"profile required installed entry unresolved: {name!r}")

        for name in ELF_REQUIRED:
            if name not in names:
                raise SystemExit(f"required ELF archive entry missing: {name}")
            require_arm_elf(zf.read(name), name)

        if "bin/pkg" not in names:
            raise SystemExit("bin/pkg must be a real archive script entry")
        pkg = zf.read("bin/pkg")
        if LEGACY_PREFIX in pkg:
            raise SystemExit("bin/pkg embeds forbidden legacy prefix")
        for marker in BRIDGE_MARKERS:
            if marker in pkg:
                raise SystemExit(f"bin/pkg contains bridge marker: {marker.decode('utf-8', 'replace')}")

        for optional in ("bin/termux-setup-package-manager", "bin/termux-change-repo"):
            if optional in names and LEGACY_PREFIX in zf.read(optional):
                raise SystemExit(f"{optional} embeds forbidden legacy prefix")

        sources = zf.read("etc/apt/sources.list")
        if b"deb " not in sources or b"https://" not in sources:
            raise SystemExit("sources.list has no HTTPS deb repository declaration")

    return {
        "schema": "rafcodephi.real-bootstrap-import-receipt/v1",
        "source_zip": str(zip_path.resolve()),
        "source_manifest": str(manifest_path.resolve()),
        "sha256": actual,
        "bytes": zip_path.stat().st_size,
        "arch": "arm",
        "package_name": PACKAGE,
        "prefix": PREFIX,
        "profile": "real-pkg",
        "bridge_allowed": False,
        "legacy_prefix_allowed": False,
        "package_repo_runtime_state": manifest.get("package_repo_runtime_state", "NOT_MEASURED"),
        "claim_allowed_device_runtime": False,
        "device_runtime_proof": "TOKEN_VAZIO",
    }


def atomic_copy(source: Path, target: Path) -> None:
    target.parent.mkdir(parents=True, exist_ok=True)
    fd, tmp_name = tempfile.mkstemp(prefix=target.name + ".", suffix=".tmp", dir=target.parent)
    tmp = Path(tmp_name)
    try:
        with os.fdopen(fd, "wb") as out, source.open("rb") as inp:
            shutil.copyfileobj(inp, out, length=1024 * 1024)
            out.flush()
            os.fsync(out.fileno())
        os.replace(tmp, target)
    finally:
        if tmp.exists():
            tmp.unlink()


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--zip", required=True, type=Path)
    parser.add_argument("--manifest", required=True, type=Path)
    parser.add_argument("--dest", default=Path("app/src/main/cpp/rewritten-bootstrap-arm.zip"), type=Path)
    parser.add_argument("--receipt", default=Path("build/reports/rafcodephi-real-bootstrap-import-arm.json"), type=Path)
    args = parser.parse_args()

    if not args.zip.is_file() or not args.manifest.is_file():
        raise SystemExit("source-built bootstrap ZIP/manifest unavailable")
    receipt = validate(args.zip, args.manifest)
    atomic_copy(args.zip, args.dest)

    args.receipt.parent.mkdir(parents=True, exist_ok=True)
    receipt["embedded_rewritten_path"] = str(args.dest)
    args.receipt.write_text(json.dumps(receipt, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(f"rafcodephi_real_bootstrap_import=PASS sha256={receipt['sha256']} bytes={receipt['bytes']}")
    print("claim_allowed_device_runtime=false")
    print("device_runtime_proof=TOKEN_VAZIO")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
