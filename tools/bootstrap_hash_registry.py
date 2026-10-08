#!/usr/bin/env python3
"""RAFCODEPHI ZIP bootstrap hash registry and deterministic candidate emitter.

This is a HOSTED CI/custody adapter; it is not an Android runtime or freestanding
core. No network, third-party Python packages, credentials, signing or trust
promotion. A registry entry alone never grants release rights.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import re
import sys
import zipfile
from pathlib import Path

ABIS = ("arm", "aarch64", "i686", "x86_64")
HEX = re.compile(r"^[0-9a-f]{64}$")
COMMIT_HEX = re.compile(r"^(?:[0-9a-f]{40}|[0-9a-f]{64})$")
SCHEMA = "rafcodephi.bootstrap-hash-registry/v1"
CANDIDATE = "rafcodephi.bootstrap-hash-candidates/v1"
MAX_ZIP = 256 * 1024 * 1024
MAX_UNCOMPRESSED = 768 * 1024 * 1024
MAX_ENTRY = 256 * 1024 * 1024
MAX_ENTRIES = 65_536
MAX_RATIO = 500


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(65_536), b""):
            digest.update(block)
    return digest.hexdigest()


def safe_name(name: str) -> bool:
    return bool(name) and len(name) <= 4096 and not (
        name.startswith("/") or name.startswith("\\") or "\\" in name
        or "\x00" in name or any(part == ".." for part in name.split("/"))
    )


def inspect(path: Path, abi: str) -> dict:
    if abi not in ABIS:
        raise ValueError("unsupported ABI: " + abi)
    if not path.is_file() or not (0 < path.stat().st_size <= MAX_ZIP):
        raise ValueError("missing/oversized bootstrap ZIP: " + str(path))
    names: set[str] = set()
    total = 0
    with zipfile.ZipFile(path) as archive:
        infos = archive.infolist()
        if not infos or len(infos) > MAX_ENTRIES:
            raise ValueError("invalid ZIP entry count")
        for info in infos:
            name = info.filename
            if not safe_name(name) or name in names:
                raise ValueError("unsafe/duplicate ZIP path: " + repr(name))
            names.add(name)
            size, comp = info.file_size, info.compress_size
            if size < 0 or size > MAX_ENTRY or comp < 0:
                raise ValueError("entry size limit: " + name)
            total += size
            if total > MAX_UNCOMPRESSED or (size and not comp) or (
                comp and size // comp > MAX_RATIO
            ):
                raise ValueError("ZIP expansion limit: " + name)
        if "SYMLINKS.txt" not in names or "BOOTSTRAP_PROFILE.json" not in names:
            raise ValueError("missing required ZIP bootstrap manifest")
        profile_bytes = archive.read("BOOTSTRAP_PROFILE.json")
        if len(profile_bytes) > 64 * 1024:
            raise ValueError("oversized bootstrap profile")
        profile = json.loads(profile_bytes)
        if profile.get("arch") != abi:
            raise ValueError("bootstrap profile ABI mismatch")
        if profile.get("claim_allowed") is not False:
            raise ValueError("unexpected bootstrap release claim")
        if archive.testzip() is not None:
            raise ValueError("ZIP CRC validation failed")
        symlink_text = archive.read("SYMLINKS.txt")
        if len(symlink_text) > 1024 * 1024:
            raise ValueError("symlinks manifest too large")
        installed = set(names)
        for row in symlink_text.decode("utf-8").splitlines():
            if not row:
                continue
            fields = row.split("←")
            if len(fields) != 2 or not fields[1]:
                raise ValueError("malformed SYMLINKS.txt")
            target = fields[1]
            while target.startswith("./"):
                target = target[2:]
            if not safe_name(target):
                raise ValueError("unsafe symlink destination")
            installed.add(target)
        if not {"bin/sh", "bin/pkg"}.issubset(installed):
            raise ValueError("first boot sh/pkg not present")
    return {
        "abi": abi, "sha256": sha256(path), "bytes": path.stat().st_size,
        "profile": profile.get("profile", "UNKNOWN"),
        "package_name": profile.get("package_name", "TOKEN_VAZIO"),
        "zip_entries": len(names), "uncompressed_bytes": total,
        "structure": "PASS", "zip_crc": "PASS",
    }


def verify_registry(registry: dict) -> None:
    if registry.get("schema") != SCHEMA or not isinstance(registry.get("entries"), list):
        raise ValueError("invalid registry schema")
    seen: set[tuple[str, str]] = set()
    for item in registry["entries"]:
        if not isinstance(item, dict):
            raise ValueError("registry entry not an object")
        abi, digest = item.get("abi"), item.get("sha256")
        if abi not in ABIS or not isinstance(digest, str) or not HEX.fullmatch(digest):
            raise ValueError("registry entry invalid ABI/SHA-256")
        key = (abi, digest)
        if key in seen:
            raise ValueError("duplicate registry ABI/SHA-256")
        seen.add(key)
        if item.get("status") != "OWNER_APPROVED":
            raise ValueError("only owner-approved entries belong in the trusted registry")
        for field in ("source_commit", "approval_ref", "license_ref", "blake3"):
            value = item.get(field)
            if not isinstance(value, str) or not value or value == "TOKEN_VAZIO":
                raise ValueError("registry missing provenance: " + field)
        if not HEX.fullmatch(item["blake3"]) or not COMMIT_HEX.fullmatch(item["source_commit"]):
            raise ValueError("registry BLAKE3/source commit invalid")
        if item.get("claim_allowed") is not False:
            raise ValueError("registry cannot automatically enable claims")


def main() -> int:
    parser = argparse.ArgumentParser()
    sub = parser.add_subparsers(dest="command", required=True)
    verify = sub.add_parser("verify")
    verify.add_argument("--registry", type=Path, required=True)
    emit = sub.add_parser("emit")
    emit.add_argument("--registry", type=Path, required=True)
    emit.add_argument("--zip", nargs=2, action="append", metavar=("ABI", "PATH"), required=True)
    emit.add_argument("--source-commit", required=True)
    emit.add_argument("--out", type=Path, required=True)
    args = parser.parse_args()
    registry = json.loads(args.registry.read_text(encoding="utf-8"))
    verify_registry(registry)
    if args.command == "verify":
        print("BOOTSTRAP_HASH_REGISTRY=PASS entries=" + str(len(registry["entries"])))
        return 0
    if not COMMIT_HEX.fullmatch(args.source_commit.lower()):
        raise ValueError("invalid source commit SHA (40/64 hex)")
    zips = dict(args.zip)
    if len(zips) != len(args.zip) or not {"arm", "aarch64"}.issubset(zips):
        raise ValueError("ARM32 and AArch64 must be provided as a complete pair")
    result = []
    known = {(x["abi"], x["sha256"]) for x in registry["entries"]}
    for abi, path in sorted(zips.items()):
        item = inspect(Path(path), abi)
        item["source_commit"] = args.source_commit.lower()
        item["registry_match"] = (abi, item["sha256"]) in known
        item["registry_approved"] = item["registry_match"]
        item["candidate_state"] = "ALREADY_REGISTERED" if item["registry_match"] else "PENDING_OWNER_REVIEW"
        item["blake3"] = "TOKEN_VAZIO"  # The build's existing BLAKE3 pin step remains authoritative.
        result.append(item)
    payload = {
        "schema": CANDIDATE, "candidate_only": True, "automatic_install": False,
        "device_proof": "TOKEN_VAZIO", "claim_allowed": False,
        "release_allowed": False, "entries": result,
    }
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print("BOOTSTRAP_ZIP_REGISTRY_CANDIDATES=PASS entries=" + str(len(result)))
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (ValueError, OSError, zipfile.BadZipFile, UnicodeDecodeError, json.JSONDecodeError) as error:
        print("BOOTSTRAP_HASH_REGISTRY=FAIL " + str(error), file=sys.stderr)
        raise SystemExit(1)
