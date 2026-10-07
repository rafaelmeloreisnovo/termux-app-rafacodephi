#!/usr/bin/env python3
"""Minimal stdlib-only runner for plain zero-argument test_* functions.

Scope is intentionally narrow: modules using pytest fixtures, parametrization or
other framework semantics are rejected rather than silently interpreted.
"""

from __future__ import annotations

import importlib.util
import inspect
import sys
import traceback
from pathlib import Path


def load_module(path: Path):
    name = f"_raf_plain_test_{path.stem}_{abs(hash(path.resolve()))}"
    spec = importlib.util.spec_from_file_location(name, path)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"cannot load test module: {path}")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def run_path(path: Path) -> tuple[int, int]:
    module = load_module(path)
    tests = [
        (name, obj)
        for name, obj in vars(module).items()
        if name.startswith("test_") and callable(obj)
    ]
    tests.sort(key=lambda item: item[0])
    if not tests:
        raise RuntimeError(f"no plain test_* functions discovered: {path}")

    passed = 0
    failed = 0
    for name, fn in tests:
        signature = inspect.signature(fn)
        if signature.parameters:
            print(
                f"BLOCKED {path}:{name}: parameters require an explicit test framework",
                file=sys.stderr,
            )
            failed += 1
            continue
        try:
            fn()
        except Exception:
            failed += 1
            print(f"FAIL {path}:{name}", file=sys.stderr)
            traceback.print_exc()
        else:
            passed += 1
            print(f"PASS {path}:{name}")
    return passed, failed


def main(argv: list[str]) -> int:
    if not argv:
        print("usage: run_plain_test_functions.py TEST_FILE [...]", file=sys.stderr)
        return 64

    total_passed = 0
    total_failed = 0
    for raw in argv:
        path = Path(raw)
        if not path.is_file():
            print(f"BLOCKED missing test module: {path}", file=sys.stderr)
            total_failed += 1
            continue
        passed, failed = run_path(path)
        total_passed += passed
        total_failed += failed

    print(f"plain_test_runner passed={total_passed} failed={total_failed}")
    return 1 if total_failed else 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
