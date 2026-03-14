#!/usr/bin/env python3
from __future__ import annotations

import re
from pathlib import Path
from typing import Dict, List, Tuple

TARGET_KEYS = [
    "TERMUX_PKG_VERSION",
    "TERMUX_PKG_HOMEPAGE",
    "TERMUX_PKG_DESCRIPTION",
]

ASSIGN_RE = re.compile(r"^\s*(?:export\s+)?(TERMUX_PKG_[A-Z0-9_]+)\s*=(.*)$")
VAR_REF_RE = re.compile(r"\$(\{?[A-Za-z_][A-Za-z0-9_]*\}?)")


def _strip_comment(line: str) -> str:
    out: List[str] = []
    in_single = False
    in_double = False
    escaped = False
    for ch in line:
        if escaped:
            out.append(ch)
            escaped = False
            continue
        if ch == "\\":
            out.append(ch)
            escaped = True
            continue
        if ch == "'" and not in_double:
            in_single = not in_single
            out.append(ch)
            continue
        if ch == '"' and not in_single:
            in_double = not in_double
            out.append(ch)
            continue
        if ch == "#" and not in_single and not in_double:
            break
        out.append(ch)
    return "".join(out)


def _ends_with_continuation(line: str) -> bool:
    # unescaped trailing backslash
    stripped = line.rstrip("\n")
    count = 0
    i = len(stripped) - 1
    while i >= 0 and stripped[i] == "\\":
        count += 1
        i -= 1
    return count % 2 == 1


def _collect_value(lines: List[str], start_idx: int, initial_value: str) -> Tuple[str, int, List[str]]:
    warnings: List[str] = []
    buf = initial_value
    i = start_idx
    in_single = False
    in_double = False
    escaped = False

    def feed(segment: str) -> None:
        nonlocal in_single, in_double, escaped
        for ch in segment:
            if escaped:
                escaped = False
                continue
            if ch == "\\" and not in_single:
                escaped = True
                continue
            if ch == "'" and not in_double:
                in_single = not in_single
            elif ch == '"' and not in_single:
                in_double = not in_double

    feed(buf)
    while True:
        cont = _ends_with_continuation(buf) and not in_single
        if cont:
            buf = buf[:-1]
        if not cont and not in_single and not in_double:
            break
        i += 1
        if i >= len(lines):
            warnings.append("unterminated_assignment")
            break
        nxt = _strip_comment(lines[i].rstrip("\n"))
        if cont:
            buf += nxt
        else:
            buf += "\n" + nxt
        feed("\n" + nxt)
    return buf, i, warnings


def _decode_value(raw: str, env: Dict[str, str], warnings: List[str]) -> str:
    value = raw.strip()
    if len(value) >= 2 and value[0] == value[-1] and value[0] in ("'", '"'):
        quote = value[0]
        value = value[1:-1]
        if quote == '"':
            value = value.replace(r"\n", "\n").replace(r'\"', '"').replace(r"\\", "\\")

    def repl(match: re.Match[str]) -> str:
        token = match.group(1)
        name = token[1:-1] if token.startswith("{") and token.endswith("}") else token
        if name in env:
            return env[name]
        warnings.append(f"unresolved_var:{name}")
        return ""

    return VAR_REF_RE.sub(repl, value)


def parse_build_sh(build_sh_path: Path) -> Dict[str, object]:
    lines = build_sh_path.read_text(encoding="utf-8", errors="ignore").splitlines(True)
    env: Dict[str, str] = {}
    warnings: List[str] = []

    i = 0
    while i < len(lines):
        raw_line = _strip_comment(lines[i].rstrip("\n"))
        m = ASSIGN_RE.match(raw_line)
        if not m:
            i += 1
            continue
        key, init_value = m.group(1), m.group(2)
        full_value, i, collect_warn = _collect_value(lines, i, init_value)
        warnings.extend(collect_warn)
        env[key] = _decode_value(full_value, env, warnings)
        i += 1

    metadata = {
        "version": env.get("TERMUX_PKG_VERSION", ""),
        "homepage": env.get("TERMUX_PKG_HOMEPAGE", ""),
        "description": env.get("TERMUX_PKG_DESCRIPTION", ""),
        "parse_mode": "dedicated_parser",
        "parse_warnings": [],
    }

    fallback_warnings: List[str] = []
    if not all(metadata[k] for k in ("version", "homepage", "description")):
        raw_plain = build_sh_path.read_text(encoding="utf-8", errors="ignore").splitlines()
        for ln in raw_plain:
            t = ln.strip()
            if t.startswith("TERMUX_PKG_VERSION=") and not metadata["version"]:
                metadata["version"] = t.split("=", 1)[1].strip().strip("\"'")
            elif t.startswith("TERMUX_PKG_HOMEPAGE=") and not metadata["homepage"]:
                metadata["homepage"] = t.split("=", 1)[1].strip().strip("\"'")
            elif t.startswith("TERMUX_PKG_DESCRIPTION=") and not metadata["description"]:
                metadata["description"] = t.split("=", 1)[1].strip().strip("\"'")
        fallback_warnings.append("fallback_simple_assignment_scan")
        metadata["parse_mode"] = "dedicated_parser+fallback"

    dedup_warn = []
    seen = set()
    for w in warnings + fallback_warnings:
        if w and w not in seen:
            dedup_warn.append(w)
            seen.add(w)
    metadata["parse_warnings"] = dedup_warn
    return metadata
