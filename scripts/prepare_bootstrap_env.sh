#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT_DIR"

MODE="--print-env"
SKIP_ANDROID_PREFLIGHT=false

usage() {
  echo "Usage: $0 [--print-env|--github-env] [--skip-android-preflight]" >&2
  echo "RAF_BOOTSTRAP_SOURCE: local | upstream | source-built-real" >&2
}

for arg in "$@"; do
  case "$arg" in
    --github-env|--print-env)
      MODE="$arg"
      ;;
    --skip-android-preflight)
      SKIP_ANDROID_PREFLIGHT=true
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      usage
      exit 1
      ;;
  esac
done

log() { printf '[prepare_bootstrap_env] %s\n' "$*" >&2; }

if [[ "$SKIP_ANDROID_PREFLIGHT" == "true" ]]; then
  log "Android preflight already satisfied by caller; skipping duplicate setup"
else
  ./scripts/ci_android_preflight.sh >&2
fi

BOOTSTRAP_SOURCE="${RAF_BOOTSTRAP_SOURCE:-local}"
PROFILE_REQUIREMENT="auto"
SOURCE_BUILT_RECEIPT=""
log "Bootstrap source: $BOOTSTRAP_SOURCE"
case "$BOOTSTRAP_SOURCE" in
  local)
    bash scripts/build_bootstrap_profile.sh >&2
    PROFILE_REQUIREMENT="required"
    ;;
  upstream)
    ./gradlew :app:downloadBootstraps --no-daemon >&2
    log "Upstream source does not receive RAFCODEPhi profile promotion automatically"
    ;;
  source-built-real)
    : "${RAF_REAL_BOOTSTRAP_ZIP_ARM:?RAF_REAL_BOOTSTRAP_ZIP_ARM is required for source-built-real}"
    : "${RAF_REAL_BOOTSTRAP_MANIFEST:?RAF_REAL_BOOTSTRAP_MANIFEST is required for source-built-real}"
    [[ -f "$RAF_REAL_BOOTSTRAP_ZIP_ARM" ]] || { log "ERROR: Missing source-built ARM bootstrap: $RAF_REAL_BOOTSTRAP_ZIP_ARM"; exit 1; }
    [[ -f "$RAF_REAL_BOOTSTRAP_MANIFEST" ]] || { log "ERROR: Missing source-built bootstrap manifest: $RAF_REAL_BOOTSTRAP_MANIFEST"; exit 1; }
    SOURCE_BUILT_RECEIPT="build/reports/rafcodephi-real-bootstrap-import-arm.json"
    python3 scripts/import_rafcodephi_real_bootstrap.py \
      --zip "$RAF_REAL_BOOTSTRAP_ZIP_ARM" \
      --manifest "$RAF_REAL_BOOTSTRAP_MANIFEST" \
      --dest app/src/main/cpp/rewritten-bootstrap-arm.zip \
      --receipt "$SOURCE_BUILT_RECEIPT" >&2
    PROFILE_REQUIREMENT="required"
    log "Source-built real ARM bootstrap imported fail-closed"
    ;;
  *)
    echo "Unsupported RAF_BOOTSTRAP_SOURCE=$BOOTSTRAP_SOURCE (allowed: local, upstream, source-built-real)" >&2
    exit 2
    ;;
esac

# Keep the canonical token consumed by the repository-wide static install
# contract, while making the strengthened same-observation scope explicit.
log "Verifying bootstrap contract: source + exact embedded rewritten archives..."
if ! RAF_BOOTSTRAP_REQUIRE_PROFILE="$PROFILE_REQUIREMENT" ./scripts/verify_bootstrap_contract.sh --check >&2; then
  log "ERROR: Bootstrap contract verification failed"
  exit 1
fi
log "Bootstrap contract OK"

if [[ "$BOOTSTRAP_SOURCE" == "local" ]]; then
  MATRIX="build/reports/bootstrap-profile-matrix.json"
  [[ -s "$MATRIX" ]] || { log "ERROR: Missing bootstrap profile matrix: $MATRIX"; exit 1; }
  MATRIX_SHA256="$(sha256sum "$MATRIX" | awk '{print $1}')"
  [[ "$MATRIX_SHA256" =~ ^[0-9a-f]{64}$ ]] || { log "ERROR: Invalid bootstrap profile matrix SHA256"; exit 1; }
  log "Bootstrap profile matrix SHA256: $MATRIX_SHA256"
elif [[ "$BOOTSTRAP_SOURCE" == "source-built-real" ]]; then
  [[ -n "$SOURCE_BUILT_RECEIPT" && -s "$SOURCE_BUILT_RECEIPT" ]] || {
    log "ERROR: source-built-real import receipt missing"
    exit 1
  }
  IMPORT_SHA256="$(sha256sum "$SOURCE_BUILT_RECEIPT" | awk '{print $1}')"
  [[ "$IMPORT_SHA256" =~ ^[0-9a-f]{64}$ ]] || { log "ERROR: Invalid source-built import receipt SHA256"; exit 1; }
  log "Source-built import receipt SHA256: $IMPORT_SHA256"
fi

if ! python3 -c 'import blake3' >/dev/null 2>&1; then
  log "Installing blake3..."
  python3 -m pip install --user blake3 >&2
fi

if HASH_OUTPUT="$(python3 - <<'PY'
from pathlib import Path
from blake3 import blake3
import hashlib
import re

# termux-bootstrap-zip.S embeds rewritten-bootstrap-*.zip. Hash the exact bytes
# returned by TermuxInstaller.getZip(), not the pre-rewrite source archives.
base = Path('app/src/main/cpp')
mapping = {
    'TERMUX_BOOTSTRAP_SHA256_AARCH64': 'rewritten-bootstrap-aarch64.zip',
    'TERMUX_BOOTSTRAP_SHA256_ARM': 'rewritten-bootstrap-arm.zip',
    'TERMUX_BOOTSTRAP_SHA256_I686': 'rewritten-bootstrap-i686.zip',
    'TERMUX_BOOTSTRAP_SHA256_X86_64': 'rewritten-bootstrap-x86_64.zip',
    'TERMUX_BOOTSTRAP_BLAKE3_AARCH64': 'rewritten-bootstrap-aarch64.zip',
    'TERMUX_BOOTSTRAP_BLAKE3_ARM': 'rewritten-bootstrap-arm.zip',
    'TERMUX_BOOTSTRAP_BLAKE3_I686': 'rewritten-bootstrap-i686.zip',
    'TERMUX_BOOTSTRAP_BLAKE3_X86_64': 'rewritten-bootstrap-x86_64.zip',
}
for env_key, file_name in mapping.items():
    path = base / file_name
    if not path.is_file():
        raise SystemExit(f"Missing embedded bootstrap archive: {path}")
    data = path.read_bytes()
    if env_key.startswith('TERMUX_BOOTSTRAP_BLAKE3_'):
        digest = blake3(data).hexdigest()
        algo = 'BLAKE3'
    else:
        digest = hashlib.sha256(data).hexdigest()
        algo = 'SHA256'
    if not re.fullmatch(r'[0-9a-f]{64}', digest):
        raise SystemExit(f"Invalid {algo} for {path}: {digest}")
    print(f"{env_key}={digest}")
PY
)"; then
  readarray -t HASH_LINES <<<"$HASH_OUTPUT"
else
  HASH_STATUS=$?
  log "ERROR: Embedded bootstrap hash generation failed (python exit ${HASH_STATUS})"
  exit "$HASH_STATUS"
fi

if [[ ${#HASH_LINES[@]} -ne 8 ]]; then
  log "ERROR: Expected 8 embedded bootstrap hash lines (BLAKE3+SHA256), got ${#HASH_LINES[@]}"
  exit 1
fi

if [[ "$MODE" == "--github-env" ]]; then
  : "${GITHUB_ENV:?GITHUB_ENV must be set for --github-env mode}"
  for line in "${HASH_LINES[@]}"; do
    echo "$line" >> "$GITHUB_ENV"
    log "Added to GITHUB_ENV: ${line%%=*}"
  done
else
  for line in "${HASH_LINES[@]}"; do
    echo "export $line"
  done
fi

log "Bootstrap environment OK (${#HASH_LINES[@]} hashes from exact embedded rewritten archives)"
