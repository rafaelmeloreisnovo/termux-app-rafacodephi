#!/usr/bin/env python3
"""Audit GitHub Actions references without confusing version existence with execution.

The policy is date-stamped and conservative. Unknown actions are reported for review;
known unsupported majors and floating branch refs fail in --strict mode. Policy
metadata records where the current major was checked, but reference validity remains
separate from workflow execution evidence.
"""

from __future__ import annotations

import argparse
import json
import os
import re
import sys
from collections import Counter
from pathlib import Path
from typing import Any

POLICY_VERIFIED_ON = "2026-09-06"
POLICY_SOURCES = {
    "actions/checkout": "https://github.com/actions/checkout/releases",
    "actions/setup-java": "https://github.com/actions/setup-java/releases",
    "actions/setup-python": "https://github.com/actions/setup-python/releases",
    "android-actions/setup-android": "https://github.com/android-actions/setup-android/releases",
    "softprops/action-gh-release": "https://github.com/softprops/action-gh-release/releases",
}
POLICY: dict[str, dict[str, Any]] = {
    # Dated policy: current majors are verified against official action repositories.
    # Older explicitly-compatible majors may remain in controlled workflows; this is
    # reference compatibility only and never proves that a workflow executed.
    "actions/checkout": {"current": 7, "compatible": {4, 6, 7}},
    "actions/setup-java": {"current": 6, "compatible": {4, 5, 6}},
    "actions/setup-python": {"current": 7, "compatible": {6, 7}},
    "actions/upload-artifact": {"current": 7, "compatible": {4, 5, 6, 7}},
    "actions/download-artifact": {"current": 8, "compatible": {4, 5, 6, 7, 8}},
    "android-actions/setup-android": {"current": 4, "compatible": {4}},
    "softprops/action-gh-release": {"current": 3, "compatible": {3}},
    "gradle/actions/wrapper-validation": {"current": 6, "compatible": {3, 4, 5, 6}},
    "gradle/actions/dependency-submission": {"current": 6, "compatible": {3, 4, 5, 6}},
    "gradle/actions/setup-gradle": {"current": 6, "compatible": {3, 4, 5, 6}},
}

USES_RE = re.compile(
    r"^\s*(?:-\s*)?uses:\s*['\"]?([^@'\"\s]+)@([^'\"\s#]+)",
    re.MULTILINE,
)
LOCAL_WORKFLOW_RE = re.compile(
    r"^\s*(?:-\s*)?uses:\s*['\"]?(\./\.github/workflows/[^'\"\s#]+)",
    re.MULTILINE,
)
VERSION_RE = re.compile(r"^v?(\d+)(?:\.\d+){0,2}(?:[-+][0-9A-Za-z.-]+)?$")
SHA_RE = re.compile(r"^[0-9a-fA-F]{40}$")
FLOATING_REFS = {"main", "master", "develop", "dev", "latest", "head"}
FAIL_STATES = {"UNSUPPORTED_MAJOR", "FLOATING_REF"}


def classify(action: str, ref: str) -> tuple[str, str]:
    """Return (state, explanation) for one action reference."""
    if action.startswith("./"):
        return "LOCAL_ACTION", "ação local"

    if SHA_RE.fullmatch(ref):
        return "PINNED_SHA", "referência imutável"

    match = VERSION_RE.fullmatch(ref)
    if match:
        major = int(match.group(1))
        rule = POLICY.get(action)
        if rule is None:
            return "UNTRACKED_ACTION", f"major v{major}; ação fora da política conhecida"
        if major == rule["current"]:
            return "CURRENT_MAJOR", f"major atual verificado em {POLICY_VERIFIED_ON}"
        if major in rule["compatible"]:
            return "COMPATIBLE_MAJOR", f"major compatível declarado; atual=v{rule['current']}"
        allowed = ",".join(f"v{x}" for x in sorted(rule["compatible"]))
        return "UNSUPPORTED_MAJOR", f"major não permitido pela política; permitidos={allowed}"

    if ref.lower() in FLOATING_REFS:
        return "FLOATING_REF", "branch/tag flutuante sem imutabilidade"

    return "UNTRACKED_REF", "referência não classificada; revisão humana necessária"


def workflow_closure(root: Path, entry: str) -> list[Path]:
    """Return the reusable-workflow source closure rooted at one workflow.

    This proves only local workflow topology. It is not execution evidence.
    """
    workflow_dir = (root / ".github" / "workflows").resolve()
    entry_path = (root / entry).resolve()
    try:
        entry_path.relative_to(workflow_dir)
    except ValueError as exc:
        raise SystemExit(f"entry is outside .github/workflows: {entry}") from exc
    if not entry_path.is_file():
        raise SystemExit(f"entry workflow not found: {entry}")

    seen: set[Path] = set()
    pending = [entry_path]
    while pending:
        path = pending.pop()
        if path in seen:
            continue
        seen.add(path)
        text = path.read_text(encoding="utf-8")
        for match in LOCAL_WORKFLOW_RE.finditer(text):
            rel = match.group(1)[2:]
            target = (root / rel).resolve()
            try:
                target.relative_to(workflow_dir)
            except ValueError as exc:
                raise SystemExit(f"local workflow edge escapes workflow root: {rel}") from exc
            if target.suffix not in {".yml", ".yaml"}:
                raise SystemExit(f"local workflow edge is not YAML: {rel}")
            if not target.is_file():
                raise SystemExit(f"local workflow edge missing: {rel}")
            if target not in seen:
                pending.append(target)
    return sorted(seen)


def audit(root: Path, paths: list[Path] | None = None) -> list[dict[str, Any]]:
    records: list[dict[str, Any]] = []
    workflow_dir = root / ".github" / "workflows"
    if paths is None:
        paths = sorted((*workflow_dir.glob("*.yml"), *workflow_dir.glob("*.yaml")))

    for path in paths:
        text = path.read_text(encoding="utf-8")
        for line_number, line in enumerate(text.splitlines(), start=1):
            match = USES_RE.match(line)
            if not match:
                continue
            action, ref = match.groups()
            state, explanation = classify(action, ref)
            records.append(
                {
                    "file": str(path.relative_to(root)),
                    "line": line_number,
                    "action": action,
                    "ref": ref,
                    "state": state,
                    "explanation": explanation,
                    "policy_source": POLICY_SOURCES.get(action),
                }
            )
    return records


def markdown_report(records: list[dict[str, Any]]) -> str:
    counts = Counter(record["state"] for record in records)
    lines = [
        "# GitHub Actions reference audit",
        "",
        f"Policy verified on: `{POLICY_VERIFIED_ON}`",
        "",
        "## Summary",
        "",
        "| State | Count |",
        "|---|---:|",
    ]
    for state, count in sorted(counts.items()):
        lines.append(f"| `{state}` | {count} |")

    lines.extend(
        [
            "",
            "## References",
            "",
            "| File:line | Action | Ref | State | Note |",
            "|---|---|---|---|---|",
        ]
    )
    for record in records:
        lines.append(
            "| `{file}:{line}` | `{action}` | `{ref}` | `{state}` | {explanation} |".format(
                **record
            )
        )
    lines.append("")
    lines.append(
        "> This audit proves only that references match the dated local policy. "
        "It does not prove that a workflow executed successfully."
    )
    return "\n".join(lines) + "\n"


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--root", default=".", help="repository root")
    parser.add_argument("--json", dest="json_path", help="write machine-readable report")
    parser.add_argument(
        "--strict",
        action="store_true",
        help="fail on known unsupported majors or floating branch refs",
    )
    parser.add_argument(
        "--reachable-from",
        help="audit only the transitive local reusable-workflow closure rooted at this path",
    )
    parser.add_argument(
        "--require-immutable",
        action="store_true",
        help="fail when an external reference in scope is not a full 40-hex commit SHA",
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    root = Path(args.root).resolve()
    scoped_paths = workflow_closure(root, args.reachable_from) if args.reachable_from else None
    records = audit(root, scoped_paths)
    report = markdown_report(records)
    print(report, end="")

    summary_path = os.environ.get("GITHUB_STEP_SUMMARY")
    if summary_path:
        with open(summary_path, "a", encoding="utf-8") as handle:
            handle.write(report)

    if args.json_path:
        output = {
            "schema_version": 2,
            "policy_verified_on": POLICY_VERIFIED_ON,
            "policy_sources": POLICY_SOURCES,
            "records": records,
            "summary": dict(Counter(record["state"] for record in records)),
            "scope": {
                "reachable_from": args.reachable_from or None,
                "workflow_count": len(scoped_paths) if scoped_paths is not None else None,
                "require_immutable": args.require_immutable,
            },
            "claim_allowed": False,
            "claim_reason": "reference audit is not workflow execution evidence",
        }
        Path(args.json_path).write_text(
            json.dumps(output, ensure_ascii=False, indent=2) + "\n",
            encoding="utf-8",
        )

    failures = [record for record in records if record["state"] in FAIL_STATES]
    if args.strict and failures:
        print(f"ERROR: {len(failures)} policy violation(s).", file=sys.stderr)
        return 1

    if args.require_immutable:
        mutable = [
            record
            for record in records
            if record["state"] not in {"PINNED_SHA", "LOCAL_ACTION"}
        ]
        if mutable:
            print(
                f"ERROR: {len(mutable)} mutable external reference(s) in governed scope.",
                file=sys.stderr,
            )
            for record in mutable:
                print(
                    f"  {record['file']}:{record['line']} "
                    f"{record['action']}@{record['ref']} [{record['state']}]",
                    file=sys.stderr,
                )
            return 1

    print(
        "OK: reference policy audit completed; execution evidence remains separate.",
        file=sys.stderr,
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
