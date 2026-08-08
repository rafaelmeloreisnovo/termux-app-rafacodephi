#!/usr/bin/env python3
"""Import a source-built RAFCODEPHI bootstrap into an exact embedded ARM slot.

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
API_RECEIVER_COMPONENT = f"{PACKAGE}.api/com.termux.api.TermuxApiReceiver"
APT_UPDATE_GUARD = "RAFCODEPHI_PACKAGE_REPOSITORY_NOT_PUBLISHED"
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
    "BOOTSTRAP_INFO",
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
    "bin/termux-battery-status",
    "bin/termux-sensor",
    "libexec/termux-api",
    "libexec/termux-api-broadcast",
    "etc/apt/sources.list.d/termux.sources",
    "etc/apt/apt.conf.d/00rafcodephi-repository-block",
    "var/lib/dpkg/status",
)
ELF_REQUIRED = (
    "bin/apt",
    "bin/apt-get",
    "bin/dpkg",
    "bin/bash",
    "bin/busybox",
    "bin/proot",
    "libexec/termux-api-broadcast",
)
ARCH_SPECS = {
    "arm": {"elf_class": 1, "machine": 40, "machine_name": "EM_ARM"},
    "aarch64": {"elf_class": 2, "machine": 183, "machine_name": "EM_AARCH64"},
}


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


def parse_key_value_payload(payload: bytes, label: str) -> dict[str, str]:
    if len(payload) > 64 * 1024:
        raise SystemExit(f"{label} exceeds 64 KiB")
    try:
        text = payload.decode("utf-8")
    except UnicodeDecodeError as exc:
        raise SystemExit(f"{label} is not UTF-8: {exc}") from exc
    out: dict[str, str] = {}
    for number, raw in enumerate(text.splitlines(), 1):
        line = raw.strip()
        if not line or line.startswith("#"):
            continue
        if "=" not in line:
            raise SystemExit(f"{label} line {number} is not key=value")
        key, value = line.split("=", 1)
        if not key or key in out:
            raise SystemExit(f"{label} key invalid or duplicated: {key!r}")
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


def require_elf(payload: bytes, label: str, arch: str) -> None:
    spec = ARCH_SPECS[arch]
    if len(payload) < 20 or payload[:4] != b"\x7fELF":
        raise SystemExit(f"{label} is not ELF")
    if payload[4] != spec["elf_class"]:
        raise SystemExit(
            f"{label} has ELF class={payload[4]}, expected {spec['elf_class']} for {arch}"
        )
    if payload[5] != 1:
        raise SystemExit(f"{label} is not little-endian ELF")
    machine = struct.unpack_from("<H", payload, 18)[0]
    if machine != spec["machine"]:
        raise SystemExit(
            f"{label} e_machine={machine}, expected {spec['machine_name']}={spec['machine']}"
        )
    if LEGACY_PREFIX in payload:
        raise SystemExit(f"{label} embeds forbidden legacy prefix")


def validate(zip_path: Path, manifest_path: Path, arch: str) -> dict[str, object]:
    manifest = parse_manifest(manifest_path)
    expected_manifest = {
        "schema": MANIFEST_SCHEMA,
        "package_name": PACKAGE,
        "prefix": PREFIX,
        "api_package": f"{PACKAGE}.api",
        "api_receiver_component": API_RECEIVER_COMPONENT,
        "api_access_control": "SIGNATURE_PERMISSION_NO_SHARED_UID",
        "bridge_allowed": "false",
        "legacy_prefix_allowed": "false",
        "termux_api_cli": "EMBEDDED",
        "package_repo_runtime_state": "BLOCKED_CUSTOM_REPOSITORY_NOT_PUBLISHED",
        "apt_update_guard": APT_UPDATE_GUARD,
        "claim_allowed_device_runtime": "false",
        "device_runtime_proof": "TOKEN_VAZIO",
    }
    for key, expected in expected_manifest.items():
        if manifest.get(key) != expected:
            raise SystemExit(f"manifest contract mismatch {key}: {manifest.get(key)!r} != {expected!r}")

    declared = manifest.get(f"sha256_{arch}", "")
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
            "arch": arch,
            "api_package": f"{PACKAGE}.api",
            "api_receiver_component": API_RECEIVER_COMPONENT,
            "api_access_control": "SIGNATURE_PERMISSION_NO_SHARED_UID",
            "claim_allowed": False,
            "release_allowed": False,
            "device_validation": "TOKEN_VAZIO",
            "real_pkg_relocation_claim_allowed": False,
            "apt_update_guard": APT_UPDATE_GUARD,
        }
        for key, expected in expected_profile.items():
            if profile.get(key) != expected:
                raise SystemExit(f"profile contract mismatch {key}: {profile.get(key)!r} != {expected!r}")

        bootstrap_info = parse_key_value_payload(zf.read("BOOTSTRAP_INFO"), "BOOTSTRAP_INFO")
        expected_info = {
            "TERMUX_PACKAGE_NAME": PACKAGE,
            "TERMUX_ARCH": arch,
            "RAFCODEPHI_BOOTSTRAP_PROFILE": "real-pkg",
            "RAFCODEPHI_PACKAGE_LAYER": "real-pkg",
            "RAFCODEPHI_DEVICE_VALIDATION": "TOKEN_VAZIO",
            "RAFCODEPHI_CLAIM_ALLOWED": "0",
            "BOOTSTRAP_FULLENGINE_READY": "0",
            "BOOTSTRAP_PKG_REAL": "1",
            "BOOTSTRAP_APT_REAL": "1",
            "BOOTSTRAP_DPKG_REAL": "1",
            "BOOTSTRAP_TERMUX_API_CLI": "1",
            "RAFCODEPHI_API_PACKAGE": f"{PACKAGE}.api",
            "RAFCODEPHI_API_RECEIVER_COMPONENT": API_RECEIVER_COMPONENT,
            "RAFCODEPHI_API_ACCESS_CONTROL": "SIGNATURE_PERMISSION_NO_SHARED_UID",
            "RAFCODEPHI_APT_UPDATE_GUARD": APT_UPDATE_GUARD,
        }
        for key, expected in expected_info.items():
            if bootstrap_info.get(key) != expected:
                raise SystemExit(
                    f"BOOTSTRAP_INFO contract mismatch {key}: "
                    f"{bootstrap_info.get(key)!r} != {expected!r}"
                )
        required_profile = profile.get("required_entries")
        if not isinstance(required_profile, list) or not required_profile:
            raise SystemExit("profile required_entries missing/empty")
        for name in required_profile:
            if not isinstance(name, str) or name not in available:
                raise SystemExit(f"profile required installed entry unresolved: {name!r}")

        elf_names: set[str] = set()
        for entry in zf.infolist():
            name = entry.filename
            if entry.is_dir():
                continue
            with zf.open(entry, "r") as stream:
                prefix = stream.read(4)
                if prefix == b"\x7fELF":
                    payload = prefix + stream.read()
                    require_elf(payload, name, arch)
                    elf_names.add(name)
                elif name.startswith(("bin/", "libexec/", "etc/apt/")):
                    payload = prefix + stream.read()
                    if LEGACY_PREFIX in payload:
                        raise SystemExit(f"{name} embeds forbidden legacy prefix")
        for name in ELF_REQUIRED:
            if name not in elf_names:
                raise SystemExit(f"required architecture-matched ELF archive entry missing: {name}")

        status = zf.read("var/lib/dpkg/status")
        if b"\nPackage: termux-api\n" not in b"\n" + status:
            raise SystemExit("dpkg status does not contain the embedded termux-api package")

        symlinks = zf.read("SYMLINKS.txt")
        if b"termux-api-broadcast\xe2\x86\x90libexec/termux-api" not in symlinks:
            raise SystemExit("termux-api compatibility symlink is missing")

        api_client = zf.read("libexec/termux-api-broadcast")
        receiver = API_RECEIVER_COMPONENT.encode()
        if receiver not in api_client:
            raise SystemExit("termux-api client does not target the RAFCODEPHI API receiver")
        for stub in (
            b"com.termux/com.termux.app.TermuxService",
            b"com.termux.service_api",
        ):
            if stub in api_client:
                raise SystemExit("termux-api client contains the removed service stub route")

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

        sources = zf.read("etc/apt/sources.list.d/termux.sources")
        if (
            b"# RAFCODEPHI_PACKAGE_REPOSITORY=BLOCKED_CUSTOM_REPOSITORY_NOT_PUBLISHED" not in sources
            or b"Enabled: no" not in sources
            or b"https://packages.rafcodephi.invalid/termux" not in sources
            or b"termux.net" in sources
        ):
            raise SystemExit("custom-prefix apt repository is not deterministically disabled")
        apt_block = zf.read("etc/apt/apt.conf.d/00rafcodephi-repository-block")
        if (
            b"APT::Update::Pre-Invoke" not in apt_block
            or b"RAFCODEPHI_PACKAGE_REPOSITORY_NOT_PUBLISHED" not in apt_block
            or b"exit 100" not in apt_block
        ):
            raise SystemExit("apt update fail-closed hook is missing or invalid")

    return {
        "schema": "rafcodephi.real-bootstrap-import-receipt/v1",
        "source_zip": str(zip_path.resolve()),
        "source_manifest": str(manifest_path.resolve()),
        "sha256": actual,
        "bytes": zip_path.stat().st_size,
        "arch": arch,
        "api_package": f"{PACKAGE}.api",
        "api_receiver_component": API_RECEIVER_COMPONENT,
        "api_access_control": "SIGNATURE_PERMISSION_NO_SHARED_UID",
        "termux_api_cli": "EMBEDDED",
        "package_name": PACKAGE,
        "prefix": PREFIX,
        "profile": "real-pkg",
        "bridge_allowed": False,
        "legacy_prefix_allowed": False,
        "package_repo_runtime_state": manifest.get("package_repo_runtime_state", "NOT_MEASURED"),
        "apt_update_guard": APT_UPDATE_GUARD,
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
    parser.add_argument("--arch", choices=tuple(ARCH_SPECS), default="arm")
    parser.add_argument("--zip", required=True, type=Path)
    parser.add_argument("--manifest", required=True, type=Path)
    parser.add_argument("--dest", type=Path)
    parser.add_argument("--receipt", type=Path)
    parser.add_argument("--validate-only", action="store_true")
    args = parser.parse_args()

    if not args.zip.is_file() or not args.manifest.is_file():
        raise SystemExit("source-built bootstrap ZIP/manifest unavailable")
    receipt = validate(args.zip, args.manifest, args.arch)
    if args.validate_only:
        print(
            f"rafcodephi_real_bootstrap_validation=PASS arch={args.arch} "
            f"sha256={receipt['sha256']} bytes={receipt['bytes']}"
        )
        print("claim_allowed_device_runtime=false")
        print("device_runtime_proof=TOKEN_VAZIO")
        return 0

    dest = args.dest or Path(f"app/src/main/cpp/rewritten-bootstrap-{args.arch}.zip")
    receipt_path = args.receipt or Path(
        f"build/reports/rafcodephi-real-bootstrap-import-{args.arch}.json"
    )
    atomic_copy(args.zip, dest)

    receipt_path.parent.mkdir(parents=True, exist_ok=True)
    receipt["embedded_rewritten_path"] = str(dest)
    receipt_path.write_text(json.dumps(receipt, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(
        f"rafcodephi_real_bootstrap_import=PASS arch={args.arch} "
        f"sha256={receipt['sha256']} bytes={receipt['bytes']}"
    )
    print("claim_allowed_device_runtime=false")
    print("device_runtime_proof=TOKEN_VAZIO")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
