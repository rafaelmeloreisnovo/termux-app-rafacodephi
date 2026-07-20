#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
CC_BIN="${CC:-cc}"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT

run_gate() {
  local name="$1"
  shift
  local log="$TMP/${name}.log"
  echo "::group::${name}"
  if "$@" >"$log" 2>&1; then
    echo "${name}: PASS"
    echo "::endgroup::"
    return 0
  fi
  local rc=$?
  echo "::error title=${name} failed::exit_code=${rc}"
  cat "$log"
  echo "::endgroup::"
  return "$rc"
}

run_gate compile_numbase \
  "$CC_BIN" \
  -std=c11 -Wall -Wextra -Werror -Os \
  -fno-common -ffunction-sections -fdata-sections \
  -I"$ROOT/rafaelia/src/main/cpp" \
  "$ROOT/rafaelia/src/main/cpp/raf_numbase.c" \
  "$ROOT/tests/native/test_raf_numbase.c" \
  -Wl,--gc-sections -lm \
  -o "$TMP/test_raf_numbase"

run_gate compile_ecc32_compact \
  "$CC_BIN" \
  -std=c11 -Wall -Wextra -Werror -Os \
  -DRAF_ECC32_FORCE_COMPACT=1 \
  -fno-common -ffunction-sections -fdata-sections \
  -I"$ROOT/rafaelia/src/main/cpp" \
  "$ROOT/tests/native/test_raf_ecc32_masked.c" \
  -Wl,--gc-sections \
  -o "$TMP/test_raf_ecc32_compact"

run_gate compile_ecc32_unrolled \
  "$CC_BIN" \
  -std=c11 -Wall -Wextra -Werror -O2 \
  -DRAF_ECC32_FORCE_UNROLL=1 \
  -fno-common -ffunction-sections -fdata-sections \
  -I"$ROOT/rafaelia/src/main/cpp" \
  "$ROOT/tests/native/test_raf_ecc32_masked.c" \
  -Wl,--gc-sections \
  -o "$TMP/test_raf_ecc32_unrolled"

# APKC's production syscall layer remains ARM-only. Host emitters explicitly
# enable RAF_APKC_HOST_TEST and write bounded fixtures for independent parsers.
run_gate compile_apkc_dex_emitter \
  "$CC_BIN" \
  -std=c11 -Wall -Wextra -Werror -Os \
  -Wno-error=unused-function -Wno-error=unused-variable \
  -fno-common -ffunction-sections -fdata-sections \
  -I"$ROOT/apkc" \
  "$ROOT/tests/native/apkc_emit_minimal_dex.c" \
  -Wl,--gc-sections \
  -o "$TMP/apkc_emit_minimal_dex"

run_gate compile_apkc_one_class_dex_emitter \
  "$CC_BIN" \
  -std=c11 -Wall -Wextra -Werror -Os \
  -Wno-error=unused-function -Wno-error=unused-variable \
  -fno-common -ffunction-sections -fdata-sections \
  -I"$ROOT/apkc" \
  "$ROOT/tests/native/apkc_emit_one_class_dex.c" \
  -Wl,--gc-sections \
  -o "$TMP/apkc_emit_one_class_dex"

run_gate compile_apkc_elf_emitter \
  "$CC_BIN" \
  -std=c11 -Wall -Wextra -Werror -Os \
  -Wno-error=unused-function -Wno-error=unused-variable \
  -fno-common -ffunction-sections -fdata-sections \
  -I"$ROOT/apkc" \
  "$ROOT/tests/native/apkc_emit_minimal_elf.c" \
  -Wl,--gc-sections \
  -o "$TMP/apkc_emit_minimal_elf"

run_gate compile_apkc_exec_elf_emitter \
  "$CC_BIN" \
  -std=c11 -Wall -Wextra -Werror -Os \
  -Wno-error=unused-function -Wno-error=unused-variable \
  -fno-common -ffunction-sections -fdata-sections \
  -I"$ROOT/apkc" \
  "$ROOT/tests/native/apkc_emit_exec_elf.c" \
  -Wl,--gc-sections \
  -o "$TMP/apkc_emit_exec_elf"

run_gate test_numbase "$TMP/test_raf_numbase"
run_gate test_ecc32_compact "$TMP/test_raf_ecc32_compact"
run_gate test_ecc32_unrolled "$TMP/test_raf_ecc32_unrolled"
run_gate emit_apkc_empty_dex "$TMP/apkc_emit_minimal_dex" "$TMP/classes-empty.dex"
run_gate emit_apkc_one_class_dex "$TMP/apkc_emit_one_class_dex" "$TMP/classes-one-class.dex"
run_gate emit_apkc_elf_rel \
  "$TMP/apkc_emit_minimal_elf" "$TMP/apkc-arm32.o" "$TMP/apkc-arm64.o"
run_gate emit_apkc_elf_exec \
  "$TMP/apkc_emit_exec_elf" "$TMP/apkc-arm32-exec" "$TMP/apkc-arm64-exec"

run_gate validate_apkc_empty_dex \
  python3 "$ROOT/scripts/validate_apkc_dex_contract.py" "$TMP/classes-empty.dex" --pretty
run_gate validate_apkc_one_class_dex \
  python3 "$ROOT/scripts/validate_apkc_one_class_dex.py" "$TMP/classes-one-class.dex" --pretty
run_gate validate_apkc_elf32_rel \
  python3 "$ROOT/scripts/validate_apkc_elf_contract.py" "$TMP/apkc-arm32.o" --expect arm32 --kind rel --pretty
run_gate validate_apkc_elf64_rel \
  python3 "$ROOT/scripts/validate_apkc_elf_contract.py" "$TMP/apkc-arm64.o" --expect arm64 --kind rel --pretty
run_gate validate_apkc_elf32_exec \
  python3 "$ROOT/scripts/validate_apkc_elf_contract.py" "$TMP/apkc-arm32-exec" --expect arm32 --kind exec --pretty
run_gate validate_apkc_elf64_exec \
  python3 "$ROOT/scripts/validate_apkc_elf_contract.py" "$TMP/apkc-arm64-exec" --expect arm64 --kind exec --pretty
run_gate validate_compiler_capability_matrix \
  python3 "$ROOT/scripts/validate_compiler_capability_matrix.py" --pretty
run_gate validate_first_part_gap_map \
  python3 "$ROOT/scripts/validate_first_part_gap_map.py" --pretty
run_gate validate_operational_coherence \
  python3 "$ROOT/scripts/validate_operational_technical_coherence.py"
run_gate validate_browser_fail_closed \
  python3 "$ROOT/scripts/validate_browser_fail_closed.py"
run_gate index_loose_artifacts \
  python3 "$ROOT/scripts/index_loose_operational_artifacts.py" --validate --summary
run_gate validate_native_gc \
  python3 "$ROOT/scripts/validate_raf_native_gc_contract.py"
run_gate test_warning_contract \
  python3 "$ROOT/tests/test_raf_compile_warning_contract.py"

echo "RAFCODE-Phi native compile contract: PASS"
