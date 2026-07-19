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

"$CC_BIN" \
  -std=c11 -Wall -Wextra -Werror -Os \
  -DRAF_ECC32_FORCE_COMPACT=1 \
  -fno-common -ffunction-sections -fdata-sections \
  -I"$ROOT/rafaelia/src/main/cpp" \
  "$ROOT/tests/native/test_raf_ecc32_masked.c" \
  -Wl,--gc-sections \
  -o "$TMP/test_raf_ecc32_compact"

"$CC_BIN" \
  -std=c11 -Wall -Wextra -Werror -O2 \
  -DRAF_ECC32_FORCE_UNROLL=1 \
  -fno-common -ffunction-sections -fdata-sections \
  -I"$ROOT/rafaelia/src/main/cpp" \
  "$ROOT/tests/native/test_raf_ecc32_masked.c" \
  -Wl,--gc-sections \
  -o "$TMP/test_raf_ecc32_unrolled"

# APKC's production syscall layer remains ARM-only. The test source explicitly
# enables RAF_APKC_HOST_TEST and emits a structural DEX for independent checks.
"$CC_BIN" \
  -std=c11 -Wall -Wextra -Werror -Os \
  -Wno-error=unused-function -Wno-error=unused-variable \
  -fno-common -ffunction-sections -fdata-sections \
  -I"$ROOT/apkc" \
  "$ROOT/tests/native/apkc_emit_minimal_dex.c" \
  -Wl,--gc-sections \
  -o "$TMP/apkc_emit_minimal_dex"

"$TMP/test_raf_numbase"
"$TMP/test_raf_ecc32_compact"
"$TMP/test_raf_ecc32_unrolled"
"$TMP/apkc_emit_minimal_dex" "$TMP/classes.dex"

python3 "$ROOT/scripts/validate_apkc_dex_contract.py" "$TMP/classes.dex" --pretty
python3 "$ROOT/scripts/validate_operational_technical_coherence.py"
python3 "$ROOT/scripts/index_loose_operational_artifacts.py" --validate --summary
python3 "$ROOT/scripts/validate_raf_native_gc_contract.py"
python3 "$ROOT/tests/test_raf_compile_warning_contract.py"

echo "RAFCODE-Phi native compile contract: PASS"
