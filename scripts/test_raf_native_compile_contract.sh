#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
CC_BIN="${CC:-cc}"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT

"$CC_BIN" \
  -std=c11 -Wall -Wextra -Werror -Os \
  -fno-common -ffunction-sections -fdata-sections \
  -I"$ROOT/rafaelia/src/main/cpp" \
  "$ROOT/rafaelia/src/main/cpp/raf_numbase.c" \
  "$ROOT/tests/native/test_raf_numbase.c" \
  -Wl,--gc-sections -lm \
  -o "$TMP/test_raf_numbase"

"$TMP/test_raf_numbase"
python3 "$ROOT/scripts/validate_raf_native_gc_contract.py"
python3 -m unittest "$ROOT/tests/test_raf_compile_warning_contract.py"

echo "RAFCODE-Phi native compile contract: PASS"
