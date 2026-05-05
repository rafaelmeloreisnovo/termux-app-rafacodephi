#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SPEC_FILE="${ROOT_DIR}/BOOTSTRAP_LOWLEVEL_RAFAELIA.txt"

log(){ printf '[bootstrap-lowlevel-sync] %s\n' "$*"; }
fail(){ printf '[bootstrap-lowlevel-sync] ERROR: %s\n' "$*" >&2; exit 1; }

[[ -f "${SPEC_FILE}" ]] || fail "Missing ${SPEC_FILE}."

cd "${ROOT_DIR}"

required=(
  "scripts/prepare_bootstrap_env.sh"
  "scripts/verify_bootstrap_contract.sh"
  "scripts/generate_developer_bootstraps.sh"
  "scripts/bootstrap_zip_contract_check.c"
  "scripts/bootstrap_zip_builder.c"
  "scripts/setup_android_toolchain.sh"
  "scripts/build_apk_matrix.sh"
  "app/build.gradle"
)

missing=0
for path in "${required[@]}"; do
  if [[ -e "$path" ]]; then
    log "OK $path"
  else
    log "MISSING $path"
    missing=1
  fi
done

if ! rg -q "SEM HEAP \| SEM MALLOC" "${SPEC_FILE}"; then
  fail "Spec header marker not found in BOOTSTRAP_LOWLEVEL_RAFAELIA.txt"
fi
if ! rg -q "SEÇÃO 1 — TIPOS PRIMITIVOS" "${SPEC_FILE}"; then
  fail "Spec section marker not found in BOOTSTRAP_LOWLEVEL_RAFAELIA.txt"
fi

# Ensure release matrix keeps both arm32+arm64 paths in official pipeline.
rg -q "arm64-v8a" scripts/build_apk_matrix.sh || fail "arm64-v8a build path missing in scripts/build_apk_matrix.sh"
rg -q "armeabi-v7a" scripts/build_apk_matrix.sh || fail "armeabi-v7a build path missing in scripts/build_apk_matrix.sh"

if [[ "$missing" -ne 0 ]]; then
  fail "Lowlevel bootstrap references are out of sync."
fi

log "Bootstrap lowlevel spec is synchronized with repository build/release chain."
