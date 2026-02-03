#!/usr/bin/env bash
set -euo pipefail

# Static checks to reduce "zombie" drift: unique IDs/authorities.

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

fail(){ echo "FAIL: $1"; exit 1; }
ok(){ echo "OK: $1"; }

# Check for known package names in Gradle files (best-effort).
if grep -RIn "applicationId" app/build.gradle* 2>/dev/null | grep -q "com.termux"; then
  if grep -RIn "applicationId" app/build.gradle* 2>/dev/null | grep -q "com.termux\.rafacodephi"; then
    ok "applicationId appears to be com.termux.rafacodephi"
  else
    fail "applicationId not set to com.termux.rafacodephi (side-by-side risk)"
  fi
else
  echo "WARN: applicationId not found (gradle layout differs)"
fi

# Authorities check (search in manifests)
if grep -RIn "android:authorities" app/src 2>/dev/null | grep -q "com.termux\.rafacodephi"; then
  ok "authorities include com.termux.rafacodephi"
else
  echo "WARN: could not confirm authorities uniqueness"
fi

ok "static checks done"
