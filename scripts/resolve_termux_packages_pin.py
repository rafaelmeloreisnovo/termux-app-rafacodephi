#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DEFAULT_CONTRACT = ROOT / "data/contracts/termux-packages-rafcodephi-pin.v1.json"
SCHEMA = "rafcodephi.termux-packages-pin/v1"
SHA_RE = re.compile(r"^[0-9a-f]{40}$")


def die(message: str) -> "NoReturn":
    print(f"[termux-packages-pin] ERROR: {message}", file=sys.stderr)
    raise SystemExit(1)


def load_contract(path: Path) -> dict:
    try:
        doc = json.loads(path.read_text())
    except Exception as exc:
        die(f"cannot read {path}: {exc}")
    if doc.get("schema") != SCHEMA:
        die(f"unexpected schema: {doc.get('schema')!r}")
    if doc.get("repository") != "https://github.com/rafaelmeloreisnovo/termux-packages.git":
        die("repository identity drift")
    if doc.get("package_name") != "com.termux.rafacodephi":
        die("package identity drift")
    if doc.get("prefix") != "/data/data/com.termux.rafacodephi/files/usr":
        die("prefix identity drift")
    if doc.get("required_abis") != ["armeabi-v7a", "arm64-v8a"]:
        die("ABI contract drift")
    return doc


def resolve(doc: dict, selector: str) -> tuple[str, str]:
    if SHA_RE.fullmatch(selector):
        return selector, "exact"
    channels = doc.get("channels") or {}
    channel = channels.get(selector)
    if not isinstance(channel, dict):
        die(f"unknown pin channel {selector!r}; expected canonical, candidate, or exact SHA")
    commit = channel.get("commit")
    if not isinstance(commit, str) or not SHA_RE.fullmatch(commit):
        die(f"invalid commit for channel {selector!r}: {commit!r}")
    if channel.get("claim_allowed") is not False:
        die(f"channel {selector!r} unexpectedly widens claim_allowed")
    if channel.get("physical_android") != "TOKEN_VAZIO":
        die(f"channel {selector!r} unexpectedly widens physical_android")
    if selector == "candidate" and channel.get("state") == "MERGED_BASELINE":
        die("candidate channel cannot masquerade as merged baseline")
    return commit, selector


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("selector", nargs="?", default="canonical")
    parser.add_argument("--contract", type=Path, default=DEFAULT_CONTRACT)
    parser.add_argument("--github-env", action="store_true")
    parser.add_argument("--json", action="store_true")
    args = parser.parse_args()

    doc = load_contract(args.contract)
    commit, channel = resolve(doc, args.selector)
    payload = {
        "schema": "rafcodephi.termux-packages-pin-resolution/v1",
        "selector": args.selector,
        "channel": channel,
        "commit": commit,
        "repository": doc["repository"],
        "package_name": doc["package_name"],
        "prefix": doc["prefix"],
        "required_abis": doc["required_abis"],
        "claim_allowed": False,
        "physical_android": "TOKEN_VAZIO",
    }

    if args.github_env:
        env_path = Path(str(__import__("os").environ.get("GITHUB_ENV", "")))
        if not str(env_path) or str(env_path) == ".":
            die("--github-env requires GITHUB_ENV")
        with env_path.open("a") as fh:
            fh.write(f"TERMUX_PACKAGES_RAF_REPO={doc['repository']}\n")
            fh.write(f"TERMUX_PACKAGES_RAF_REF={commit}\n")
            fh.write(f"TERMUX_PACKAGES_SHA={commit}\n")
            fh.write(f"TERMUX_PACKAGES_PIN_CHANNEL={channel}\n")
    if args.json:
        print(json.dumps(payload, indent=2, sort_keys=True))
    elif not args.github_env:
        print(commit)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
