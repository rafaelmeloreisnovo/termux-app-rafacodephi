#!/usr/bin/env python3
"""Validate and transactionally import the RAFCODEPHI local-pkg ARM pair.

The producer receipt is authoritative only for source/build identity.  This
consumer revalidates ZIP structure, ABI, embedded repository hashes and the
fail-closed external repository before replacing the app's embedded ARM slots.
Device/pkg runtime remains TOKEN_VAZIO.
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
from pathlib import Path, PurePosixPath

PACKAGE = "com.termux.rafacodephi"
PREFIX = f"/data/data/{PACKAGE}/files/usr"
PRODUCER_SCHEMA = "rafcodephi.local-pkg-candidate/v1"
PAIR_SCHEMA = "rafcodephi.local-pkg-import-pair/v1"
LOCAL_STATE = "LOCAL_HASH_BOUND_TEST_CHANNEL"
LEGACY = b"/data/data/com.termux/files/usr"
ARCH = {
    "arm": {"class": 1, "machine": 40},
    "aarch64": {"class": 2, "machine": 183},
}
REQUIRED_PACKAGES = {"apt", "dpkg", "proot", "termux-tools", "nano", "python", "git"}
REQUIRED_ENTRIES = (
    "BOOTSTRAP_PROFILE.json",
    "BOOTSTRAP_INFO",
    "SYMLINKS.txt",
    "bin/sh",
    "bin/bash",
    "bin/apt",
    "bin/apt-get",
    "bin/dpkg",
    "bin/pkg",
    "bin/pkg.termux-original",
    "bin/proot",
    "bin/proot.real",
    "lib/libapt-pkg.so",
    "etc/apt/rafcodephi-local.list",
    "etc/apt/apt.conf.d/00rafcodephi-repository-block",
    "var/lib/dpkg/status",
)


def sha256_bytes(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def sha256_path(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for block in iter(lambda: f.read(1 << 20), b""):
            h.update(block)
    return h.hexdigest()


def safe_name(name: str) -> None:
    p = PurePosixPath(name)
    if not name or name.startswith("/") or "\\" in name or any(part in ("", ".", "..") for part in p.parts):
        raise RuntimeError(f"UNSAFE_ZIP_ENTRY:{name!r}")


def parse_symlinks(zf: zipfile.ZipFile) -> set[str]:
    raw = zf.read("SYMLINKS.txt")
    if len(raw) > 1024 * 1024:
        raise RuntimeError("SYMLINKS_TOO_LARGE")
    links: set[str] = set()
    for number, line in enumerate(raw.decode("utf-8").splitlines(), 1):
        if not line:
            continue
        parts = line.split("←")
        if len(parts) != 2 or not parts[0] or not parts[1]:
            raise RuntimeError(f"BAD_SYMLINK_LINE:{number}")
        target, link = parts
        safe_name(link)
        if LEGACY.decode() in target:
            raise RuntimeError(f"LEGACY_SYMLINK_TARGET:{link}")
        if link in links:
            raise RuntimeError(f"DUPLICATE_SYMLINK:{link}")
        links.add(link)
    return links


def require_elf(payload: bytes, arch: str, label: str) -> None:
    spec = ARCH[arch]
    if len(payload) < 20 or payload[:4] != b"\x7fELF":
        raise RuntimeError(f"NOT_ELF:{label}")
    if payload[4] != spec["class"] or payload[5] != 1:
        raise RuntimeError(f"ELF_CLASS_ENDIAN:{label}")
    machine = struct.unpack_from("<H", payload, 18)[0]
    if machine != spec["machine"]:
        raise RuntimeError(f"ELF_MACHINE:{label}:{machine}")
    if LEGACY in payload:
        raise RuntimeError(f"LEGACY_PREFIX_ELF:{label}")


def resolve_libapt(names: set[str]) -> str:
    exact = "lib/libapt-pkg.so"
    if exact in names:
        return exact
    candidates = sorted(n for n in names if n.startswith("lib/libapt-pkg.so."))
    if not candidates:
        raise RuntimeError("LIBAPT_MISSING")
    return candidates[0]


def parse_packages_index(data: bytes) -> list[dict[str, str]]:
    try:
        text = data.decode("utf-8")
    except UnicodeDecodeError as exc:
        raise RuntimeError("PACKAGES_INDEX_NOT_UTF8") from exc
    records: list[dict[str, str]] = []
    current: dict[str, str] = {}
    key: str | None = None
    for raw in text.splitlines() + [""]:
        if not raw:
            if current:
                records.append(current)
                current = {}
                key = None
            continue
        if raw.startswith((" ", "\t")) and key:
            current[key] += "\n" + raw
            continue
        if ":" not in raw:
            raise RuntimeError(f"PACKAGES_BAD_LINE:{raw[:80]}")
        key, value = raw.split(":", 1)
        current[key] = value.lstrip()
    return records


def validate_repo(zf: zipfile.ZipFile, names: set[str], arch: str) -> dict:
    index_name = f"var/lib/rafcodephi/repo/dists/stable/main/binary-{arch}/Packages"
    release_name = "var/lib/rafcodephi/repo/dists/stable/Release"
    if index_name not in names or release_name not in names:
        raise RuntimeError("LOCAL_REPO_INDEX_OR_RELEASE_MISSING")
    index_data = zf.read(index_name)
    records = parse_packages_index(index_data)
    package_names: set[str] = set()
    checked = 0
    for record in records:
        package = record.get("Package", "")
        filename = record.get("Filename", "")
        digest = record.get("SHA256", "")
        size_s = record.get("Size", "")
        architecture = record.get("Architecture", "")
        if not package or not filename.startswith("pool/") or len(digest) != 64 or not size_s.isdigit():
            raise RuntimeError(f"LOCAL_REPO_BAD_RECORD:{package or '<missing>'}")
        if architecture not in (arch, "all"):
            raise RuntimeError(f"LOCAL_REPO_ARCH_MISMATCH:{package}:{architecture}")
        entry = "var/lib/rafcodephi/repo/" + filename
        safe_name(entry)
        if entry not in names:
            raise RuntimeError(f"LOCAL_REPO_POOL_MISSING:{entry}")
        payload = zf.read(entry)
        if len(payload) != int(size_s) or sha256_bytes(payload) != digest.lower():
            raise RuntimeError(f"LOCAL_REPO_POOL_HASH_OR_SIZE:{package}")
        package_names.add(package)
        checked += 1
    missing = sorted(REQUIRED_PACKAGES - package_names)
    if missing:
        raise RuntimeError("LOCAL_REPO_REQUIRED_PACKAGES_MISSING:" + ",".join(missing))
    release = zf.read(release_name).decode("utf-8", "strict")
    index_sha = sha256_bytes(index_data)
    if index_sha not in release or str(len(index_data)) not in release:
        raise RuntimeError("LOCAL_REPO_RELEASE_HASH_BINDING_MISSING")
    return {"package_count": checked, "packages_sha256": index_sha}


def validate_one(zip_path: Path, receipt_path: Path, arch: str) -> dict:
    receipt = json.loads(receipt_path.read_text(encoding="utf-8"))
    if receipt.get("schema") != PRODUCER_SCHEMA or receipt.get("state") != "STRUCTURAL_CANDIDATE":
        raise RuntimeError(f"PRODUCER_RECEIPT_STATE:{arch}")
    if receipt.get("arch") != arch or receipt.get("package_name") != PACKAGE or receipt.get("prefix") != PREFIX:
        raise RuntimeError(f"PRODUCER_RECEIPT_IDENTITY:{arch}")
    if receipt.get("repository_mode") != LOCAL_STATE:
        raise RuntimeError(f"PRODUCER_REPOSITORY_MODE:{arch}")
    actual = sha256_path(zip_path)
    if receipt.get("candidate_zip_sha256") != actual:
        raise RuntimeError(f"PRODUCER_ZIP_HASH_MISMATCH:{arch}")
    if receipt.get("claim_allowed_pkg_runtime") is not False or receipt.get("device_validation") != "TOKEN_VAZIO":
        raise RuntimeError(f"PRODUCER_CLAIM_BOUNDARY:{arch}")

    with zipfile.ZipFile(zip_path, "r") as zf:
        if zf.testzip() is not None:
            raise RuntimeError(f"ZIP_CRC:{arch}")
        raw_names = zf.namelist()
        if len(raw_names) != len(set(raw_names)):
            raise RuntimeError(f"ZIP_DUPLICATES:{arch}")
        for name in raw_names:
            safe_name(name)
        names = set(raw_names)
        links = parse_symlinks(zf)
        available = names | links
        missing_entries = [x for x in REQUIRED_ENTRIES if x not in available and x != "lib/libapt-pkg.so"]
        if missing_entries:
            raise RuntimeError(f"REQUIRED_ENTRIES:{arch}:" + ",".join(missing_entries))
        resolve_libapt(names)

        profile = json.loads(zf.read("BOOTSTRAP_PROFILE.json").decode("utf-8"))
        expected = {
            "schema": "rafcodephi-bootstrap-profile/v1",
            "profile": "real-pkg",
            "package_name": PACKAGE,
            "prefix": PREFIX,
            "arch": arch,
            "claim_allowed": False,
            "release_allowed": False,
            "device_validation": "TOKEN_VAZIO",
            "local_pkg_test_channel": LOCAL_STATE,
            "local_pkg_repository": f"file:{PREFIX}/var/lib/rafcodephi/repo",
        }
        for key, value in expected.items():
            if profile.get(key) != value:
                raise RuntimeError(f"PROFILE_MISMATCH:{arch}:{key}:{profile.get(key)!r}")

        pkg = zf.read("bin/pkg")
        for marker in (b"RAFCODEPHI local-pkg adapter v1", b"Dir::Etc::sourcelist", b"rafcodephi-local.list"):
            if marker not in pkg:
                raise RuntimeError(f"PKG_ADAPTER_MARKER_MISSING:{arch}:{marker!r}")
        if LEGACY in pkg:
            raise RuntimeError(f"PKG_ADAPTER_LEGACY:{arch}")

        for elf in ("bin/apt", "bin/apt-get", "bin/dpkg", "bin/proot.real"):
            require_elf(zf.read(elf), arch, elf)

        local_list = zf.read("etc/apt/rafcodephi-local.list").decode("utf-8")
        expected_source = f"deb [trusted=yes] file:{PREFIX}/var/lib/rafcodephi/repo stable main"
        if expected_source not in local_list or "http://" in local_list or "https://" in local_list:
            raise RuntimeError(f"LOCAL_LIST_CONTRACT:{arch}")

        # The persistent source remains deliberately fail-closed; pkg opts into
        # the local source explicitly. This preserves the existing runtime guard.
        source_name = "etc/apt/sources.list.d/termux.sources"
        if source_name not in names:
            raise RuntimeError(f"PERSISTENT_BLOCKED_SOURCE_MISSING:{arch}")
        persistent = zf.read(source_name).decode("utf-8", "replace")
        if "Enabled: no" not in persistent or "packages.rafcodephi.invalid" not in persistent:
            raise RuntimeError(f"PERSISTENT_SOURCE_NOT_FAIL_CLOSED:{arch}")
        block = zf.read("etc/apt/apt.conf.d/00rafcodephi-repository-block").decode("utf-8", "replace")
        if "RAFCODEPHI_PACKAGE_REPOSITORY_NOT_PUBLISHED" not in block or "exit 100" not in block:
            raise RuntimeError(f"PERSISTENT_APT_BLOCK_MISSING:{arch}")

        repo = validate_repo(zf, names, arch)

    return {
        "arch": arch,
        "zip": str(zip_path),
        "zip_sha256": actual,
        "producer_receipt": str(receipt_path),
        "producer_receipt_sha256": sha256_path(receipt_path),
        "local_repo": repo,
        "structural_state": "PASS",
        "claim_allowed_pkg_runtime": False,
        "claim_allowed_device_runtime": False,
        "device_validation": "TOKEN_VAZIO",
    }


def transactional_replace(rows: list[tuple[Path, Path]]) -> None:
    staged: list[tuple[Path, Path]] = []
    backups: list[tuple[Path, Path]] = []
    try:
        for src, dest in rows:
            dest.parent.mkdir(parents=True, exist_ok=True)
            fd, tmp_s = tempfile.mkstemp(prefix=dest.name + ".", suffix=".new", dir=dest.parent)
            os.close(fd)
            tmp = Path(tmp_s)
            shutil.copy2(src, tmp)
            if sha256_path(tmp) != sha256_path(src):
                raise RuntimeError(f"STAGE_COPY_HASH_MISMATCH:{dest}")
            staged.append((tmp, dest))
        for _, dest in staged:
            if dest.exists():
                backup = dest.with_name(dest.name + ".raf-lkg")
                if backup.exists():
                    backup.unlink()
                os.replace(dest, backup)
                backups.append((backup, dest))
        for tmp, dest in staged:
            os.replace(tmp, dest)
    except Exception:
        for tmp, _ in staged:
            if tmp.exists():
                tmp.unlink()
        for backup, dest in reversed(backups):
            if dest.exists():
                dest.unlink()
            if backup.exists():
                os.replace(backup, dest)
        raise
    else:
        for backup, _ in backups:
            if backup.exists():
                backup.unlink()


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--arm-zip", required=True, type=Path)
    ap.add_argument("--arm-receipt", required=True, type=Path)
    ap.add_argument("--aarch64-zip", required=True, type=Path)
    ap.add_argument("--aarch64-receipt", required=True, type=Path)
    ap.add_argument("--dest-arm", required=True, type=Path)
    ap.add_argument("--dest-aarch64", required=True, type=Path)
    ap.add_argument("--report", required=True, type=Path)
    ap.add_argument("--validate-only", action="store_true")
    args = ap.parse_args()

    arm = validate_one(args.arm_zip, args.arm_receipt, "arm")
    a64 = validate_one(args.aarch64_zip, args.aarch64_receipt, "aarch64")
    if not args.validate_only:
        transactional_replace([
            (args.arm_zip, args.dest_arm),
            (args.aarch64_zip, args.dest_aarch64),
        ])
        if sha256_path(args.dest_arm) != arm["zip_sha256"] or sha256_path(args.dest_aarch64) != a64["zip_sha256"]:
            raise RuntimeError("POST_IMPORT_HASH_MISMATCH")

    payload = {
        "schema": PAIR_SCHEMA,
        "state": "VALIDATED_ONLY" if args.validate_only else "IMPORTED_STRUCTURAL",
        "package_name": PACKAGE,
        "prefix": PREFIX,
        "architectures": {"arm": arm, "aarch64": a64},
        "pair_complete": True,
        "rollback": "transactional restore of previous embedded pair on import failure",
        "claim_allowed_pkg_runtime": False,
        "claim_allowed_device_runtime": False,
        "device_validation": "TOKEN_VAZIO",
        "next_required_action": "BUILD_APK_THEN_REQUIRE_REAL_PKG_DEVICE_SMOKE",
    }
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(payload, sort_keys=True, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(payload, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
