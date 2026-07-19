#!/usr/bin/env python3
"""Translate GCC/Clang diagnostics into auditable RAFCODE-Phi actions.

Warnings are evidence about source intent; they are not deletion commands.
This tool maps each diagnostic to a safe next action and never edits source.
"""

from __future__ import annotations

import argparse
import json
import re
import sys
from dataclasses import asdict, dataclass
from pathlib import Path
from typing import Iterable

DIAGNOSTIC_RE = re.compile(
    r"^(?P<path>.*?):(?P<line>\d+):(?P<column>\d+):\s*"
    r"(?P<level>warning|error|note):\s*(?P<message>.*?)"
    r"(?:\s+\[(?P<option>-W[^\]]+)\])?$"
)


@dataclass(frozen=True)
class Diagnostic:
    path: str
    line: int
    column: int
    level: str
    message: str
    option: str
    category: str
    action: str
    severity: str
    auto_delete: bool = False


RULES: tuple[tuple[tuple[str, ...], str, str, str], ...] = (
    (
        ("-Wunused-function", "-Wunused-variable", "-Wunused-const-variable"),
        "GC_CANDIDATE",
        "Confirm internal linkage; keep warning visible; let section GC remove it or delete only after reference audit.",
        "review",
    ),
    (
        ("-Wunused-parameter",),
        "INTENTIONAL_VOID_OR_API_FIX",
        "For a stable ABI, mark the parameter with RAF_DISCARD/(void); for a private function, remove or redesign it.",
        "review",
    ),
    (
        ("-Wunused-result", "-Wignored-qualifiers"),
        "RESULT_MUST_BE_HANDLED",
        "Handle the result explicitly. RAF_DISCARD is allowed only after the ignored outcome is proven intentional.",
        "blocker",
    ),
    (
        ("-Wunreachable-code", "-Wunreachable-code-break", "-Wunreachable-code-return"),
        "UNREACHABLE_CFG",
        "Inspect control flow; annotate a proven terminal path with RAF_NORETURN/RAF_UNREACHABLE, otherwise repair it.",
        "review",
    ),
    (
        ("-Winfinite-recursion", "-Wreturn-type", "-Wimplicit-function-declaration"),
        "CONTROL_FLOW_OR_ABI_FAILURE",
        "Do not optimize around this diagnostic; repair control flow or declarations before release.",
        "blocker",
    ),
    (
        ("-Wempty-body", "-Wmisleading-indentation"),
        "LOOP_OR_BRANCH_REVIEW",
        "Verify that the empty loop/branch is intentional; use RAF_SPIN_FOREVER for terminal loops or remove dead structure.",
        "review",
    ),
    (
        ("-Wmissing-field-initializers",),
        "EXPLICIT_INITIALIZATION",
        "Confirm zero-initialization semantics and document the omitted fields; do not hide structural omissions globally.",
        "review",
    ),
)


def classify(option: str, level: str) -> tuple[str, str, str]:
    if level == "error":
        return (
            "COMPILER_ERROR",
            "Repair the compiler error; no optimization transformation is authorized.",
            "blocker",
        )
    for options, category, action, severity in RULES:
        if option in options:
            return category, action, severity
    return (
        "SOURCE_REVIEW",
        "Preserve the diagnostic and review it in source context; no automatic deletion is authorized.",
        "review",
    )


def parse_lines(lines: Iterable[str]) -> list[Diagnostic]:
    diagnostics: list[Diagnostic] = []
    for raw in lines:
        match = DIAGNOSTIC_RE.match(raw.rstrip("\n"))
        if not match:
            continue
        fields = match.groupdict()
        option = fields.get("option") or ""
        category, action, severity = classify(option, fields["level"])
        diagnostics.append(
            Diagnostic(
                path=fields["path"],
                line=int(fields["line"]),
                column=int(fields["column"]),
                level=fields["level"],
                message=fields["message"],
                option=option,
                category=category,
                action=action,
                severity=severity,
            )
        )
    return diagnostics


def build_report(diagnostics: list[Diagnostic]) -> dict[str, object]:
    categories: dict[str, int] = {}
    severities: dict[str, int] = {}
    for diagnostic in diagnostics:
        categories[diagnostic.category] = categories.get(diagnostic.category, 0) + 1
        severities[diagnostic.severity] = severities.get(diagnostic.severity, 0) + 1

    blockers = severities.get("blocker", 0)
    return {
        "schema": "raf.compile-warning-contract.v1",
        "diagnostics_total": len(diagnostics),
        "categories": dict(sorted(categories.items())),
        "severities": dict(sorted(severities.items())),
        "release_allowed": blockers == 0,
        "automatic_source_deletion": False,
        "diagnostics": [asdict(item) for item in diagnostics],
    }


def read_input(path: str | None) -> list[str]:
    if path is None or path == "-":
        return sys.stdin.readlines()
    return Path(path).read_text(encoding="utf-8", errors="replace").splitlines(True)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("log", nargs="?", default="-", help="compiler log, or - for stdin")
    parser.add_argument("--pretty", action="store_true", help="pretty-print JSON")
    parser.add_argument(
        "--fail-on",
        choices=("never", "blocker", "any"),
        default="blocker",
        help="exit policy",
    )
    args = parser.parse_args()

    diagnostics = parse_lines(read_input(args.log))
    report = build_report(diagnostics)
    json.dump(report, sys.stdout, ensure_ascii=False, indent=2 if args.pretty else None)
    sys.stdout.write("\n")

    if args.fail_on == "any" and diagnostics:
        return 1
    if args.fail_on == "blocker" and report["severities"].get("blocker", 0):
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
