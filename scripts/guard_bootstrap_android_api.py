#!/usr/bin/env python3
"""Fail-closed host gate for the real ARM32/ARM64 Termux bootstrap pair.

Only checks the startup/package executables in the *actual archive bytes*.
This is a static Android ELF API/import check; it is NOT an Android E2E test.
No installation, package downloads, mutations, or external Python libraries.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile
import zipfile

CRITICAL = (
    "bin/bash", "bin/dash", "bin/apt", "bin/apt-get", "bin/dpkg",
    "bin/busybox", "bin/proot", "bin/ninja", "libexec/termux-api-broadcast",
)
ARCH = {"arm": ("ARM", 1), "aarch64": ("AArch64", 2)}
PREFIX = "/data/data/com.termux.rafacodephi/files/usr"
SCRIPT_INTERPRETERS = {
    "bin/pkg": (PREFIX + "/bin/bash").encode("ascii"),
    "bin/login": (PREFIX + "/bin/sh").encode("ascii"),
}
API_NOTE = re.compile(
    r"\bAndroid\b[\s\S]{0,160}?description data:\s*((?:[0-9a-fA-F]{2}\s+){4})"
)
# The original device failure was specifically this Bionic symbol.
LATE_IMPORT = re.compile(r"\bmemfd_create(?:@LIBC_R)?\b")


def readelf(binary: bytes, *args: str) -> str:
    with tempfile.TemporaryDirectory(prefix="raf-elf-api-") as tmp:
        elf = Path(tmp) / "input.elf"
        elf.write_bytes(binary)
        proc = subprocess.run(
            ["readelf", "--wide", *args, str(elf)], capture_output=True,
            text=True, check=False, timeout=15,
        )
        if proc.returncode:
            raise ValueError("READELF_FAILED: " + proc.stderr.strip()[:200])
        return proc.stdout


def inspect_elf(binary: bytes, arch: str, name: str, max_api: int) -> dict:
    item = {"entry": name, "arch": arch, "state": "BLOCKED",
            "android_min_api": None, "reasons": []}
    if not binary.startswith(b"\x7fELF"):
        item["reasons"].append("NOT_ELF")
        return item
    if len(binary) < 6 or binary[4] != ARCH[arch][1]:
        item["reasons"].append("ELF_CLASS_MISMATCH")
        return item
    try:
        header = readelf(binary, "-h")
        if not re.search(r"^\s*Machine:\s*" + re.escape(ARCH[arch][0]) + r"\s*$",
                         header, re.MULTILINE):
            item["reasons"].append("ELF_MACHINE_MISMATCH")
        notes = readelf(binary, "-n")
        m = API_NOTE.search(notes)
        if not m or ".note.android.ident" not in notes:
            item["reasons"].append("ANDROID_API_NOTE_MISSING")
        else:
            api = int.from_bytes(bytes.fromhex(m.group(1)), "little")
            item["android_min_api"] = api
            if api > max_api:
                item["reasons"].append("ANDROID_MIN_API_EXCEEDS_TARGET")
        dynsym = readelf(binary, "--dyn-syms")
        if any(" UND " in row and LATE_IMPORT.search(row)
               for row in dynsym.splitlines()):
            item["reasons"].append("UNRESOLVED_MEMFD_CREATE")
    except (ValueError, OSError, subprocess.TimeoutExpired) as exc:
        item["reasons"].append("ELF_INSPECTION_ERROR:" + str(exc)[:180])
    if not item["reasons"]:
        item["state"] = "PASS_SCOPED_STATIC"
    return item


def inspect_script(binary: bytes, arch: str, name: str) -> dict:
    expected = b"#!" + SCRIPT_INTERPRETERS[name]
    head = binary.split(b"\n", 1)[0]
    reasons = [] if head == expected else ["SHEBANG_PREFIX_MISMATCH"]
    return {"entry": name, "arch": arch, "state": (
        "PASS_SCOPED_STATIC" if not reasons else "BLOCKED"),
        "android_min_api": None, "reasons": reasons}


def inspect_archive(arch: str, filename: Path, max_api: int) -> dict:
    result = {"arch": arch, "archive": filename.name, "sha256": None,
              "state": "BLOCKED", "checks": [], "reasons": []}
    try:
        if not filename.is_file():
            raise ValueError("BOOTSTRAP_ARCHIVE_MISSING")
        with filename.open("rb") as source:
            result["sha256"] = hashlib.file_digest(source, "sha256").hexdigest()
        with zipfile.ZipFile(filename) as boot:
            names = boot.namelist()
            if len(set(names)) != len(names):
                raise ValueError("DUPLICATE_ARCHIVE_ENTRIES")
            if len(names) > 10000:
                raise ValueError("ARCHIVE_ENTRY_LIMIT")
            profile_info = boot.getinfo("BOOTSTRAP_PROFILE.json")
            if profile_info.file_size > 64 * 1024:
                raise ValueError("PROFILE_SIZE_LIMIT")
            try:
                profile = json.loads(boot.read("BOOTSTRAP_PROFILE.json"))
            except UnicodeError as exc:
                raise ValueError("PROFILE_INVALID_ENCODING") from exc
            if not isinstance(profile, dict):
                raise ValueError("PROFILE_NOT_OBJECT")
            if profile.get("arch") != arch:
                raise ValueError("ARCH_PROFILE_MISMATCH")
            if profile.get("package_name") != "com.termux.rafacodephi":
                raise ValueError("PACKAGE_PROFILE_MISMATCH")
            for name in CRITICAL:
                if name not in names:
                    result["checks"].append(
                        {"entry": name, "arch": arch, "state": "BLOCKED",
                         "android_min_api": None, "reasons": ["REQUIRED_ELF_MISSING"]}
                    )
                    continue
                info = boot.getinfo(name)
                if info.file_size > 32 * 1024 * 1024:
                    result["checks"].append(
                        {"entry": name, "arch": arch, "state": "BLOCKED",
                         "android_min_api": None, "reasons": ["ELF_SIZE_LIMIT"]}
                    )
                    continue
                result["checks"].append(inspect_elf(boot.read(name), arch, name, max_api))
            for name in SCRIPT_INTERPRETERS:
                if name not in names:
                    result["checks"].append({
                        "entry": name, "arch": arch, "state": "BLOCKED",
                        "android_min_api": None, "reasons": ["REQUIRED_SCRIPT_MISSING"]
                    })
                    continue
                if boot.getinfo(name).file_size > 256 * 1024:
                    result["checks"].append({
                        "entry": name, "arch": arch, "state": "BLOCKED",
                        "android_min_api": None, "reasons": ["SCRIPT_SIZE_LIMIT"]
                    })
                    continue
                result["checks"].append(inspect_script(boot.read(name), arch, name))
            if all(x["state"] == "PASS_SCOPED_STATIC" for x in result["checks"]):
                result["state"] = "PASS_SCOPED_STATIC"
    except (OSError, zipfile.BadZipFile, KeyError, ValueError,
            UnicodeError, RuntimeError) as exc:
        result["reasons"].append(str(exc)[:200])
    return result


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--arm", required=True, type=Path)
    parser.add_argument("--aarch64", required=True, type=Path)
    parser.add_argument("--max-api", type=int, default=29)
    parser.add_argument("--out", required=True, type=Path)
    args = parser.parse_args(argv)
    if not 21 <= args.max_api <= 35:
        parser.error("--max-api must be between 21 and 35")
    results = [inspect_archive(arch, path, args.max_api) for arch, path in
               (("arm", args.arm), ("aarch64", args.aarch64))]
    passed = sum(x["state"] == "PASS_SCOPED_STATIC" for x in results)
    doc = {
        "schema": "rafcodephi.bootstrap-elf-android-api/v1",
        "scope": "STATIC_BOOTSTRAP_CRITICAL_ELFS_AND_SHEBANGS",
        "device_api_target": args.max_api,
        "archives": results,
        "state": "PASS_SCOPED_STATIC" if passed == len(results) else "BLOCKED",
        "device_execution": "TOKEN_VAZIO",
        "claim_allowed": False,
        "release_allowed": False,
    }
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_text(json.dumps(doc, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    message = f"BOOTSTRAP_ELF_ANDROID_API={doc['state']} target_api={args.max_api} archives={len(results)}"
    print(message)
    for archive in results:
        for item in archive["checks"]:
            if item["state"] != "PASS_SCOPED_STATIC":
                print(f"  {archive['arch']}:{item['entry']}: {','.join(item['reasons'])} api={item['android_min_api']}")
        for reason in archive["reasons"]:
            print(f"  {archive['arch']}: {reason}")
    summary = os.environ.get("GITHUB_STEP_SUMMARY")
    if summary:
        with open(summary, "a", encoding="utf-8") as f:
            f.write(f"### 🧭 Bootstrap ELF/API {doc['state']}\n\n"
                    f"Target Android API: **{args.max_api}**; physical E2E: **TOKEN_VAZIO**.\n\n")
            for archive in results:
                blocked = [x['entry'] for x in archive["checks"]
                           if x["state"] != "PASS_SCOPED_STATIC"]
                f.write(f"- {archive['arch']}: {archive['state']}; critical blocked: "
                        f"{', '.join(blocked) if blocked else 'none'}\n")
    return 0 if doc["state"] == "PASS_SCOPED_STATIC" else 1


if __name__ == "__main__":
    sys.exit(main())
