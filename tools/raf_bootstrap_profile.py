#!/usr/bin/env python3
from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path, PurePosixPath
import sys
import tempfile
import zipfile

SCHEMA = "rafcodephi-bootstrap-profile/v1"
PROFILE_FILE = "BOOTSTRAP_PROFILE.json"
INFO_FILE = "BOOTSTRAP_INFO"
SYMLINKS_FILE = "SYMLINKS.txt"
LEGACY_PREFIX = b"/data/data/com.termux/files/usr"
PROFILES = {"bridge", "real-pkg"}
ARCHES = {"arm", "aarch64", "i686", "x86_64"}
BASE_REQUIRED = (
    INFO_FILE, SYMLINKS_FILE, "bin/sh", "bin/pkg", "bin/apt",
    "bin/apt-get", "bin/busybox", "bin/proot",
)
REAL_REQUIRED = BASE_REQUIRED + ("bin/dpkg", "var/lib/dpkg/status")
APT_SOURCE_LEGACY = "etc/apt/sources.list"
APT_SOURCE_DEB822 = "etc/apt/sources.list.d/termux.sources"
APT_UPDATE_BLOCK = "etc/apt/apt.conf.d/00rafcodephi-repository-block"
REPOSITORY_BLOCKED = "BLOCKED_CUSTOM_REPOSITORY_NOT_PUBLISHED"
BRIDGE_MARKERS = (
    b"RAFCODEPHI pkg bridge", b"RAFCODEPHI apt bridge",
    b"real apt backend is not installed yet",
    b"real apt/apt-get backend is not installed yet",
)


class ProfileError(RuntimeError):
    pass


def sha256_file(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as fh:
        for chunk in iter(lambda: fh.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()


def safe_name(name: str) -> None:
    p = PurePosixPath(name)
    if not name or name.startswith("/") or "\\" in name or any(x in ("", ".", "..") for x in p.parts):
        raise ProfileError(f"unsafe zip entry: {name!r}")


def read_entries(path: Path) -> list[tuple[zipfile.ZipInfo, bytes]]:
    seen: set[str] = set()
    result: list[tuple[zipfile.ZipInfo, bytes]] = []
    total = 0
    with zipfile.ZipFile(path) as zf:
        for info in zf.infolist():
            safe_name(info.filename)
            if info.filename in seen:
                raise ProfileError(f"duplicate zip entry: {info.filename}")
            seen.add(info.filename)
            total += info.file_size
            if info.file_size > 512 * 1024 * 1024 or total > 2 * 1024 * 1024 * 1024:
                raise ProfileError("bootstrap expansion limit exceeded")
            result.append((info, zf.read(info.filename)))
    return result


def parse_info(data: bytes) -> dict[str, str]:
    out: dict[str, str] = {}
    for line in data.decode("utf-8").splitlines():
        if line and not line.startswith("#") and "=" in line:
            key, value = line.split("=", 1)
            out[key] = value
    return out


def parse_symlink_destinations(data: bytes) -> set[str]:
    if len(data) > 1024 * 1024:
        raise ProfileError("SYMLINKS.txt exceeds 1 MiB")
    try:
        text = data.decode("utf-8")
    except UnicodeDecodeError as exc:
        raise ProfileError("SYMLINKS.txt is not UTF-8") from exc
    destinations: set[str] = set()
    for number, line in enumerate(text.splitlines(), 1):
        if not line:
            continue
        parts = line.split("←")
        if len(parts) != 2 or not parts[0] or not parts[1]:
            raise ProfileError(f"malformed SYMLINKS.txt line {number}: {line!r}")
        target, link = parts
        safe_name(link)
        if link in destinations:
            raise ProfileError(f"duplicate symlink destination: {link}")
        if target.startswith("/") and LEGACY_PREFIX.decode() in target:
            raise ProfileError(f"legacy prefix in symlink target: {target}")
        destinations.add(link)
    if not destinations:
        raise ProfileError("SYMLINKS.txt contains no symlinks")
    return destinations


def installed_entries(zf: zipfile.ZipFile) -> set[str]:
    names = set(zf.namelist())
    if SYMLINKS_FILE not in names:
        return names
    destinations = parse_symlink_destinations(zf.read(SYMLINKS_FILE))
    conflicts = destinations & names
    if conflicts:
        raise ProfileError("symlink destination conflicts with archive entry: " + ", ".join(sorted(conflicts)))
    return names | destinations


def encode_info(values: dict[str, str]) -> bytes:
    return "".join(f"{key}={values[key]}\n" for key in sorted(values)).encode()


def clone(info: zipfile.ZipInfo) -> zipfile.ZipInfo:
    out = zipfile.ZipInfo(info.filename, info.date_time)
    out.compress_type = info.compress_type
    out.comment, out.extra = info.comment, info.extra
    out.internal_attr, out.external_attr = info.internal_attr, info.external_attr
    out.create_system, out.flag_bits = info.create_system, info.flag_bits
    return out


def manifest(
    profile: str,
    arch: str,
    package: str,
    source_repo: str,
    source_sha: str,
    apt_source: str = APT_SOURCE_LEGACY,
) -> dict:
    return {
        "schema": SCHEMA,
        "profile": profile,
        "package_layer": profile,
        "package_name": package,
        "prefix": f"/data/data/{package}/files/usr",
        "arch": arch,
        "source_repository": source_repo,
        "source_zip_sha256": source_sha,
        "required_entries": list(REAL_REQUIRED + (apt_source,) if profile == "real-pkg" else BASE_REQUIRED),
        "legacy_prefix_forbidden": profile == "real-pkg",
        "bridge_markers_forbidden": profile == "real-pkg",
        "structural_validation": "CANDIDATE",
        "device_validation": "TOKEN_VAZIO",
        "claim_allowed": False,
        "release_allowed": False,
    }


def materialize(path: Path, *, profile: str, arch: str, package_name: str, source_repo: str) -> dict:
    if profile not in PROFILES or arch not in ARCHES:
        raise ProfileError("unsupported profile or arch")
    if not path.is_file() or "/" in package_name or package_name.startswith("."):
        raise ProfileError("invalid zip or package name")

    source_sha = sha256_file(path)
    entries = read_entries(path)
    by_name = {info.filename: data for info, data in entries}
    if INFO_FILE not in by_name:
        raise ProfileError(f"missing {INFO_FILE}")

    info = parse_info(by_name[INFO_FILE])
    real = profile == "real-pkg"
    info.update({
        "RAFCODEPHI_BOOTSTRAP_PROFILE": profile,
        "RAFCODEPHI_PACKAGE_LAYER": profile,
        "RAFCODEPHI_DEVICE_VALIDATION": "TOKEN_VAZIO",
        "RAFCODEPHI_CLAIM_ALLOWED": "0",
        "BOOTSTRAP_FULLENGINE_READY": "0",
        "BOOTSTRAP_PKG_REAL": "1" if real else "0",
        "BOOTSTRAP_APT_REAL": "1" if real else "0",
        "BOOTSTRAP_DPKG_REAL": "1" if real else "0",
    })
    apt_source = APT_SOURCE_DEB822 if APT_SOURCE_DEB822 in by_name else APT_SOURCE_LEGACY
    profile_data = (json.dumps(
        manifest(profile, arch, package_name, source_repo, source_sha, apt_source),
        sort_keys=True, indent=2
    ) + "\n").encode()

    fd, tmp_name = tempfile.mkstemp(prefix=path.name + ".", suffix=".tmp", dir=path.parent)
    os.close(fd)
    tmp = Path(tmp_name)
    try:
        with zipfile.ZipFile(tmp, "w", allowZip64=True) as out:
            for old, data in entries:
                if old.filename == PROFILE_FILE:
                    continue
                out.writestr(clone(old), encode_info(info) if old.filename == INFO_FILE else data)
            zi = zipfile.ZipInfo(PROFILE_FILE, (1980, 1, 1, 0, 0, 0))
            zi.create_system, zi.external_attr = 3, 0o100600 << 16
            out.writestr(zi, profile_data)
        with zipfile.ZipFile(tmp) as check:
            if check.testzip() is not None:
                raise ProfileError("zip CRC verification failed")
        os.chmod(tmp, 0o644)
        os.replace(tmp, path)
    finally:
        if tmp.exists():
            tmp.unlink()

    report = validate(path, expected_profile=profile, expected_arch=arch, package_name=package_name)
    report.update(materialized=True, source_zip_sha256=source_sha, output_zip_sha256=sha256_file(path))
    return report


def first(zf: zipfile.ZipFile, name: str, limit: int = 65536) -> bytes:
    with zf.open(name) as fh:
        return fh.read(limit)


def scan_legacy(zf: zipfile.ZipFile) -> str | None:
    for entry in zf.infolist():
        if entry.is_dir():
            continue
        with zf.open(entry) as fh:
            carry = b""
            while chunk := fh.read(1 << 20):
                data = carry + chunk
                if LEGACY_PREFIX in data:
                    return entry.filename
                carry = data[-(len(LEGACY_PREFIX) - 1):]
    return None


def validate(path: Path, *, expected_profile: str | None, expected_arch: str | None, package_name: str | None) -> dict:
    with zipfile.ZipFile(path) as zf:
        names = zf.namelist()
        if len(names) != len(set(names)):
            raise ProfileError("duplicate entries")
        for name in names:
            safe_name(name)
        if PROFILE_FILE not in names:
            raise ProfileError(f"missing {PROFILE_FILE}")
        profile_doc = json.loads(zf.read(PROFILE_FILE))
        profile, arch, package = (profile_doc.get(k) for k in ("profile", "arch", "package_name"))
        if profile_doc.get("schema") != SCHEMA or profile not in PROFILES or arch not in ARCHES:
            raise ProfileError("invalid profile manifest")
        if expected_profile and profile != expected_profile:
            raise ProfileError(f"profile mismatch: expected={expected_profile} actual={profile}")
        if expected_arch and arch != expected_arch:
            raise ProfileError(f"arch mismatch: expected={expected_arch} actual={arch}")
        if package_name and package != package_name:
            raise ProfileError("package mismatch")
        if profile_doc.get("prefix") != f"/data/data/{package}/files/usr":
            raise ProfileError("prefix mismatch")
        if profile_doc.get("claim_allowed") is not False or profile_doc.get("release_allowed") is not False:
            raise ProfileError("claims must remain closed")
        if profile_doc.get("device_validation") != "TOKEN_VAZIO":
            raise ProfileError("device evidence must remain TOKEN_VAZIO")

        available = installed_entries(zf)
        required = REAL_REQUIRED if profile == "real-pkg" else BASE_REQUIRED
        missing = [name for name in required if name not in available]
        if missing:
            raise ProfileError("missing installed entries: " + ", ".join(missing))
        apt_source = next(
            (name for name in (APT_SOURCE_DEB822, APT_SOURCE_LEGACY) if name in available),
            None,
        )
        if profile == "real-pkg" and apt_source is None:
            raise ProfileError("real-pkg has no apt repository definition")
        declared_required = profile_doc.get("required_entries")
        if not isinstance(declared_required, list) or not declared_required:
            raise ProfileError("profile required_entries missing")
        unresolved = [name for name in declared_required if not isinstance(name, str) or name not in available]
        if unresolved:
            raise ProfileError("profile required entries unresolved: " + ", ".join(map(str, unresolved)))

        info = parse_info(zf.read(INFO_FILE))
        if info.get("RAFCODEPHI_BOOTSTRAP_PROFILE") != profile or info.get("RAFCODEPHI_PACKAGE_LAYER") != profile:
            raise ProfileError("BOOTSTRAP_INFO profile mismatch")
        if info.get("RAFCODEPHI_CLAIM_ALLOWED") != "0" or info.get("BOOTSTRAP_FULLENGINE_READY") != "0":
            raise ProfileError("BOOTSTRAP_INFO claim boundary violated")

        classifications = {}
        for name in ("bin/pkg", "bin/apt", "bin/apt-get"):
            if name not in names:
                raise ProfileError(f"required archive entry missing: {name}")
            data = first(zf, name)
            classifications[name] = (
                "ELF" if data.startswith(b"\x7fELF")
                else "SCRIPT_BRIDGE" if any(m in data for m in BRIDGE_MARKERS)
                else "SCRIPT_OR_OTHER"
            )

        if profile == "bridge":
            if b"RAFCODEPHI pkg bridge" not in first(zf, "bin/pkg"):
                raise ProfileError("pkg bridge marker missing")
            if b"RAFCODEPHI apt bridge" not in first(zf, "bin/apt"):
                raise ProfileError("apt bridge marker missing")
        else:
            for name in ("bin/apt", "bin/apt-get", "bin/dpkg"):
                if first(zf, name, 4) != b"\x7fELF":
                    raise ProfileError(f"real-pkg requires ELF: {name}")
            for name in ("bin/pkg", "bin/apt", "bin/apt-get"):
                if any(m in first(zf, name) for m in BRIDGE_MARKERS):
                    raise ProfileError(f"bridge marker in {name}")
            if not any(n == "lib/libapt-pkg.so" or n.startswith("lib/libapt-pkg.so.") for n in names):
                raise ProfileError("libapt-pkg missing")
            sources = zf.read(apt_source).decode(errors="replace")
            if profile_doc.get("package_repo_runtime_state") == REPOSITORY_BLOCKED:
                if apt_source != APT_SOURCE_DEB822:
                    raise ProfileError("blocked custom repository must use the Deb822 source contract")
                required_source_tokens = (
                    "# RAFCODEPHI_PACKAGE_REPOSITORY=BLOCKED_CUSTOM_REPOSITORY_NOT_PUBLISHED",
                    "Enabled: no",
                    "URIs: https://packages.rafcodephi.invalid/termux",
                )
                if any(token not in sources for token in required_source_tokens) or "termux.net" in sources:
                    raise ProfileError("custom-prefix apt repository is not deterministically disabled")
                if APT_UPDATE_BLOCK not in names:
                    raise ProfileError("apt update fail-closed hook missing")
                block = zf.read(APT_UPDATE_BLOCK).decode(errors="replace")
                if "RAFCODEPHI_PACKAGE_REPOSITORY_NOT_PUBLISHED" not in block or "exit 100" not in block:
                    raise ProfileError("apt update fail-closed hook invalid")
            elif not any(
                line.strip() and not line.lstrip().startswith("#") and
                ("http://" in line or "https://" in line)
                for line in sources.splitlines()
            ):
                raise ProfileError("apt source has no repository")
            legacy = scan_legacy(zf)
            if legacy:
                raise ProfileError(f"legacy prefix in {legacy}")

    return {
        "schema": SCHEMA, "zip": str(path), "zip_sha256": sha256_file(path),
        "profile": profile, "arch": arch, "package_name": package,
        "classifications": classifications, "structural_state": "PASS",
        "device_validation": "TOKEN_VAZIO", "claim_allowed": False, "release_allowed": False,
    }


def inspect(path: Path) -> dict:
    with zipfile.ZipFile(path) as zf:
        names = zf.namelist()
        doc = json.loads(zf.read(PROFILE_FILE)) if PROFILE_FILE in names else None
        info = parse_info(zf.read(INFO_FILE)) if INFO_FILE in names else {}
    return {
        "zip": str(path), "zip_sha256": sha256_file(path), "entries": len(names),
        "profile_manifest": doc,
        "bootstrap_info_profile": info.get("RAFCODEPHI_BOOTSTRAP_PROFILE", "TOKEN_VAZIO"),
        "bootstrap_info_package_layer": info.get("RAFCODEPHI_PACKAGE_LAYER", "TOKEN_VAZIO"),
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    sub = parser.add_subparsers(dest="command", required=True)
    p = sub.add_parser("materialize")
    p.add_argument("--zip", required=True, type=Path)
    p.add_argument("--profile", required=True, choices=sorted(PROFILES))
    p.add_argument("--arch", required=True, choices=sorted(ARCHES))
    p.add_argument("--package-name", default="com.termux.rafacodephi")
    p.add_argument("--source-repo", default="TOKEN_VAZIO")
    p = sub.add_parser("validate")
    p.add_argument("--zip", required=True, type=Path)
    p.add_argument("--expected-profile", choices=sorted(PROFILES))
    p.add_argument("--expected-arch", choices=sorted(ARCHES))
    p.add_argument("--package-name")
    p = sub.add_parser("inspect")
    p.add_argument("--zip", required=True, type=Path)
    args = parser.parse_args()
    try:
        if args.command == "materialize":
            report = materialize(args.zip, profile=args.profile, arch=args.arch,
                                 package_name=args.package_name, source_repo=args.source_repo)
        elif args.command == "validate":
            report = validate(args.zip, expected_profile=args.expected_profile,
                              expected_arch=args.expected_arch, package_name=args.package_name)
        else:
            report = inspect(args.zip)
    except (ProfileError, OSError, ValueError, zipfile.BadZipFile, json.JSONDecodeError) as exc:
        print(json.dumps({"state": "FAIL", "error": str(exc)}, sort_keys=True), file=sys.stderr)
        return 1
    print(json.dumps(report, sort_keys=True, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
