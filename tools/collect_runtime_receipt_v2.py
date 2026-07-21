#!/usr/bin/env python3
"""Collect a non-destructive, bounded Termux/Android runtime receipt."""
from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
from typing import Any
from datetime import datetime, timezone

SCHEMA = "termux.rafacodephi.runtime_receipt.v2"
REPOSITORY = "rafaelmeloreisnovo/termux-app-rafacodephi"
TOKEN_VAZIO = "TOKEN_VAZIO"
HEX40 = re.compile(r"^[0-9a-f]{40}$")
COMMANDS = ("sh", "ls", "pkg", "apt", "dpkg", "proot")


def sha256_bytes(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def canonical_digest(payload: dict[str, Any]) -> str:
    material = dict(payload)
    material.pop("receipt_sha256", None)
    encoded = json.dumps(
        material,
        ensure_ascii=False,
        sort_keys=True,
        separators=(",", ":"),
    ).encode("utf-8")
    return sha256_bytes(encoded)


def run_bounded(argv: list[str], timeout: float = 5.0) -> tuple[int, bytes, bytes]:
    try:
        completed = subprocess.run(
            argv,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            timeout=timeout,
            check=False,
        )
        return completed.returncode, completed.stdout, completed.stderr
    except (OSError, subprocess.TimeoutExpired) as exc:
        code = 124 if isinstance(exc, subprocess.TimeoutExpired) else 127
        return code, b"", str(exc).encode("utf-8", "replace")


def preview(data: bytes, limit: int = 160) -> str:
    text = (
        data.decode("utf-8", "replace")
        .replace("\x00", " ")
        .replace("\r", " ")
        .replace("\n", " ")
    )
    return text[:limit] if text else TOKEN_VAZIO


def probe_command(name: str) -> dict[str, Any]:
    path = shutil.which(name)
    if not path:
        return {
            "name": name,
            "path": TOKEN_VAZIO,
            "exists": False,
            "executable": False,
            "probe_exit_code": 127,
            "stdout_sha256": TOKEN_VAZIO,
            "stderr_sha256": TOKEN_VAZIO,
            "version_preview": TOKEN_VAZIO,
        }
    argv = [path, "-c", "printf shell-ok"] if name == "sh" else [path, "--version"]
    code, out, err = run_bounded(argv)
    return {
        "name": name,
        "path": path,
        "exists": True,
        "executable": os.access(path, os.X_OK),
        "probe_exit_code": int(code),
        "stdout_sha256": sha256_bytes(out),
        "stderr_sha256": sha256_bytes(err),
        "version_preview": preview(out or err),
    }


def getprop(key: str) -> str:
    path = shutil.which("getprop")
    if not path:
        return TOKEN_VAZIO
    code, out, _ = run_bounded([path, key])
    value = out.decode("utf-8", "replace").strip()
    return value if code == 0 and value else TOKEN_VAZIO


def hash_file(path: str | None) -> str:
    if not path:
        return TOKEN_VAZIO
    target = Path(path)
    if not target.is_file():
        return TOKEN_VAZIO
    digest = hashlib.sha256()
    with target.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def git_commit(candidate: str | None) -> str:
    if candidate and HEX40.fullmatch(candidate):
        return candidate
    git = shutil.which("git")
    if git:
        code, out, _ = run_bounded([git, "rev-parse", "HEAD"])
        value = out.decode("ascii", "ignore").strip().lower()
        if code == 0 and HEX40.fullmatch(value):
            return value
    return TOKEN_VAZIO


def termux_info_probe() -> dict[str, Any]:
    path = shutil.which("termux-info")
    if not path:
        return {
            "present": False,
            "exit_code": 127,
            "stdout_sha256": TOKEN_VAZIO,
            "stderr_sha256": TOKEN_VAZIO,
            "preview": TOKEN_VAZIO,
        }
    code, out, err = run_bounded([path], timeout=15.0)
    return {
        "present": True,
        "exit_code": int(code),
        "stdout_sha256": sha256_bytes(out),
        "stderr_sha256": sha256_bytes(err),
        "preview": preview(out or err),
    }


def build_receipt(args: argparse.Namespace) -> dict[str, Any]:
    commands = [probe_command(name) for name in COMMANDS]
    kernel_code, kernel_out, kernel_err = run_bounded(
        [shutil.which("uname") or "uname", "-a"]
    )
    device = {
        "manufacturer": getprop("ro.product.manufacturer"),
        "model": getprop("ro.product.model"),
        "device": getprop("ro.product.device"),
        "android_release": getprop("ro.build.version.release"),
        "sdk_int": getprop("ro.build.version.sdk"),
        "abi_primary": getprop("ro.product.cpu.abi"),
        "abi_list": getprop("ro.product.cpu.abilist"),
        "kernel_exit_code": int(kernel_code),
        "kernel_sha256": sha256_bytes(kernel_out) if kernel_out else TOKEN_VAZIO,
        "kernel_preview": preview(kernel_out or kernel_err),
    }
    android_target = (
        device["model"] != TOKEN_VAZIO
        and device["abi_primary"] != TOKEN_VAZIO
    )
    producer_commit = git_commit(args.producer_commit)
    apk_sha256 = hash_file(args.apk_path)
    termux_info = termux_info_probe()
    prefix = os.environ.get("PREFIX") or TOKEN_VAZIO

    complete = all(
        [
            android_target,
            producer_commit != TOKEN_VAZIO,
            apk_sha256 != TOKEN_VAZIO,
            prefix != TOKEN_VAZIO,
            termux_info["present"],
            termux_info["exit_code"] == 0,
            all(item["exists"] and item["executable"] for item in commands[:2]),
        ]
    )
    if complete:
        evidence_state = "DEVICE_RECEIPT_COMPLETE"
    elif android_target:
        evidence_state = "DEVICE_OBSERVED_INCOMPLETE"
    else:
        evidence_state = "HOST_SIMULATION"

    receipt: dict[str, Any] = {
        "schema": SCHEMA,
        "version": 2,
        "authority": REPOSITORY,
        "adapter_state": "IMPLEMENTED",
        "evidence_state": evidence_state,
        "execution_evidence_usable": bool(complete),
        "claim_allowed": False,
        "runtime_claim_scope": "device environment and bounded command probes only",
        "observed_at": datetime.now(timezone.utc)
        .replace(microsecond=0)
        .isoformat()
        .replace("+00:00", "Z"),
        "producer_repository": REPOSITORY,
        "producer_commit": producer_commit,
        "package_name": args.package_name,
        "apk_path_observed": args.apk_path or TOKEN_VAZIO,
        "apk_sha256": apk_sha256,
        "prefix": prefix,
        "device": device,
        "termux_info": termux_info,
        "commands": commands,
        "install_or_mutation_performed": False,
        "rollback": (
            "No mutation was performed; retain the previous APK, app data backup "
            "and producer commit."
        ),
        "next_action": (
            "Archive this receipt with the exact APK and producer commit."
            if complete
            else (
                "Run on the target Android device with --producer-commit and "
                "--apk-path, then archive the receipt and APK hash."
            )
        ),
        "receipt_sha256": TOKEN_VAZIO,
    }
    receipt["receipt_sha256"] = canonical_digest(receipt)
    return receipt


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("output", type=Path)
    parser.add_argument(
        "--producer-commit",
        default=os.environ.get("PRODUCER_COMMIT"),
    )
    parser.add_argument("--apk-path", default=os.environ.get("APK_PATH"))
    parser.add_argument(
        "--package-name",
        default=os.environ.get("TERMUX_PACKAGE_NAME", "com.termux.rafacodephi"),
    )
    args = parser.parse_args()
    receipt = build_receipt(args)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(
        json.dumps(receipt, ensure_ascii=False, indent=2, sort_keys=True) + "\n",
        encoding="utf-8",
    )
    print(args.output)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
