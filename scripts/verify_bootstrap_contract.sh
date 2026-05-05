#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT_DIR"

MIN_FREE_MB="${MIN_FREE_MB:-1024}"
BOOTSTRAP_DIR="app/src/main/cpp"
BOOTSTRAPS=(
  "bootstrap-aarch64.zip"
  "bootstrap-arm.zip"
  "bootstrap-i686.zip"
  "bootstrap-x86_64.zip"
)

usage() {
  cat <<'USAGE'
Usage:
  bash scripts/verify_bootstrap_contract.sh --prepare
  bash scripts/verify_bootstrap_contract.sh --check
  bash scripts/verify_bootstrap_contract.sh --runtime-prefix-only

Options:
  --prepare              Run :app:downloadBootstraps then validate archives + hashes + runtime check.
  --check                Validate existing bootstrap archives + hashes + runtime check.
  --runtime-prefix-only  Validate runtime PREFIX contract only.

Environment:
  MIN_FREE_MB            Minimum free space in MB required for bootstrap checks (default: 1024).
USAGE
}

log() { printf '[bootstrap-contract] %s\n' "$*"; }
fail() { printf '[bootstrap-contract] ERROR: %s\n' "$*" >&2; exit 1; }

check_free_space() {
  local free_mb
  free_mb="$(df -Pm "$ROOT_DIR" | awk 'NR==2{print $4}')"
  [[ "$free_mb" =~ ^[0-9]+$ ]] || fail "Unable to parse free disk space: $free_mb"
  if (( free_mb < MIN_FREE_MB )); then
    fail "Free space ${free_mb}MB below required MIN_FREE_MB=${MIN_FREE_MB}MB"
  fi
  log "Free space check OK: ${free_mb}MB >= ${MIN_FREE_MB}MB"
}

check_zip_valid() {
  local zip_file="$1"
  [[ -f "$zip_file" ]] || fail "Missing bootstrap archive: $zip_file"
  [[ -s "$zip_file" ]] || fail "Bootstrap archive is empty: $zip_file"
  python3 - "$zip_file" <<'PY'
import sys, zipfile
path = sys.argv[1]
with zipfile.ZipFile(path, 'r') as zf:
    bad = zf.testzip()
    if bad is not None:
        raise SystemExit(f"Corrupted zip entry in {path}: {bad}")
    if len(zf.infolist()) == 0:
        raise SystemExit(f"Empty zip archive: {path}")
PY
}

emit_hashes() {
  local has_blake3=0
  if python3 -c 'import blake3' >/dev/null 2>&1; then
    has_blake3=1
  fi

  python3 - "$BOOTSTRAP_DIR" "$has_blake3" "${BOOTSTRAPS[@]}" <<'PY'
import hashlib
import sys
from pathlib import Path

base = Path(sys.argv[1])
has_blake3 = int(sys.argv[2])
files = sys.argv[3:]
if has_blake3:
    from blake3 import blake3

for file_name in files:
    path = base / file_name
    data = path.read_bytes()
    sha = hashlib.sha256(data).hexdigest()
    print(f"SHA256 {file_name} {sha}")
    if has_blake3:
        b3 = blake3(data).hexdigest()
        print(f"BLAKE3 {file_name} {b3}")
PY

  if (( has_blake3 == 0 )); then
    log "Python module 'blake3' unavailable; skipped BLAKE3 emission. SHA256 emitted for all archives."
  fi
}

check_bootstraps() {
  check_free_space
  for f in "${BOOTSTRAPS[@]}"; do
    check_zip_valid "$BOOTSTRAP_DIR/$f"
    log "Zip validation OK: $BOOTSTRAP_DIR/$f"
  done
  emit_hashes
}

check_runtime_prefix() {
  local runtime_prefix="${PREFIX:-${TERMUX_PREFIX:-}}"
  if [[ -z "$runtime_prefix" ]]; then
    log "PREFIX/TERMUX_PREFIX not set; runtime check skipped (build environment mode)."
    return 0
  fi

  [[ -d "$runtime_prefix" ]] || fail "PREFIX directory not found: $runtime_prefix"
  [[ -x "$runtime_prefix/bin/sh" ]] || fail "Missing runtime shell: $runtime_prefix/bin/sh"
  [[ -x "$runtime_prefix/bin/pkg" ]] || fail "Missing runtime pkg: $runtime_prefix/bin/pkg"
  log "Runtime PREFIX contract OK: $runtime_prefix"
}

mode="${1:-}"
case "$mode" in
  --prepare)
    check_free_space
    ./gradlew :app:downloadBootstraps --no-daemon
    check_bootstraps
    check_runtime_prefix
    ;;
  --check)
    check_bootstraps
    check_runtime_prefix
    ;;
  --runtime-prefix-only)
    check_runtime_prefix
    ;;
  *)
    usage
    [[ -n "$mode" ]] && exit 1 || exit 0
    ;;
esac
