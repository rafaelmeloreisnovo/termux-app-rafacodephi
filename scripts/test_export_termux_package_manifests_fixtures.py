#!/usr/bin/env python3
from __future__ import annotations

import tempfile
from pathlib import Path

import pytest

from termux_build_parser import parse_build_sh

FIXTURES = [
    {
        "name": "quoted_and_refs",
        "build_sh": """
TERMUX_PKG_BASE_VERSION="1.2.3"
TERMUX_PKG_VERSION="${TERMUX_PKG_BASE_VERSION}-r1"
TERMUX_PKG_HOMEPAGE='https://example.org/project'
TERMUX_PKG_DESCRIPTION="Tool for ${TERMUX_PKG_BASE_VERSION}"
""",
        "expect": {
            "version": "1.2.3-r1",
            "homepage": "https://example.org/project",
            "description": "Tool for 1.2.3",
            "parse_mode": "dedicated_parser",
        },
    },
    {
        "name": "multiline_continuation",
        "build_sh": """
TERMUX_PKG_VERSION=2.0.0
TERMUX_PKG_HOMEPAGE="https://example.net" \

TERMUX_PKG_DESCRIPTION="Linha 1 \
linha 2"
""",
        "expect": {
            "version": "2.0.0",
            "homepage": "https://example.net",
            "description": "Linha 1 linha 2",
            "parse_mode": "dedicated_parser",
        },
    },
    {
        "name": "fallback_missing_expr",
        "build_sh": """
TERMUX_PKG_VERSION=$UNDEFINED_VERSION
TERMUX_PKG_HOMEPAGE=https://fallback.example
TERMUX_PKG_DESCRIPTION='fallback path'
""",
        "expect": {
            "version": "$UNDEFINED_VERSION",
            "homepage": "https://fallback.example",
            "description": "fallback path",
            "parse_mode": "dedicated_parser+fallback",
        },
    },
]


def _fixture_ids():
    return [f["name"] for f in FIXTURES]


@pytest.mark.parametrize("fixture", FIXTURES, ids=_fixture_ids())
def test_parse_build_sh(fixture, tmp_path):
    p = tmp_path / f"{fixture['name']}.sh"
    p.write_text(fixture["build_sh"].lstrip("\n"), encoding="utf-8")
    result = parse_build_sh(p)
    for key, expected_value in fixture["expect"].items():
        assert result.get(key) == expected_value, (
            f"[{fixture['name']}] {key}: expected={expected_value!r} got={result.get(key)!r}"
        )


def main() -> int:
    """Direct-run entry point (kept for backwards compatibility)."""
    failures = []
    with tempfile.TemporaryDirectory() as tmp:
        root = Path(tmp)
        for fixture in FIXTURES:
            p = root / f"{fixture['name']}.sh"
            p.write_text(fixture["build_sh"].lstrip("\n"), encoding="utf-8")
            result = parse_build_sh(p)
            for key, expected_value in fixture["expect"].items():
                if result.get(key) != expected_value:
                    failures.append((fixture["name"], key, expected_value, result.get(key)))

    if failures:
        for name, key, expected, got in failures:
            print(f"[FAIL] {name} {key}: expected={expected!r} got={got!r}")
        return 1

    print(f"[OK] fixtures={len(FIXTURES)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
