#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

echo "== validate_upstream_parallel =="

# Quick static checks (no build required)
if ! grep -R --line-number -E 'applicationId\s+"com\.termux\.rafacodephi"' app/build.gradle app/build.gradle.kts 2>/dev/null | grep -q .; then
  echo "FAIL: applicationId com.termux.rafacodephi not found in app build files" >&2
  exit 1
fi

# Ensure no providers/services still reference com.termux official package (heuristic)
BAD=$(grep -R --line-number -E 'com\.termux(\.|\")' app/src/main 2>/dev/null | grep -v 'com.termux.rafacodephi' || true)
if [ -n "$BAD" ]; then
  echo "WARN: found references to com.termux (review):"
  echo "$BAD" | head -n 50
fi

echo "OK"
