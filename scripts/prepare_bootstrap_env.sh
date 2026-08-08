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
SOURCE_BUILT_RECEIPT_MATRIX=""
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
    : "${RAF_REAL_BOOTSTRAP_ZIP_AARCH64:?RAF_REAL_BOOTSTRAP_ZIP_AARCH64 is required for source-built-real}"
    : "${RAF_REAL_BOOTSTRAP_MANIFEST:?RAF_REAL_BOOTSTRAP_MANIFEST is required for source-built-real}"
    [[ -f "$RAF_REAL_BOOTSTRAP_ZIP_ARM" ]] || { log "ERROR: Missing source-built ARM bootstrap: $RAF_REAL_BOOTSTRAP_ZIP_ARM"; exit 1; }
    [[ -f "$RAF_REAL_BOOTSTRAP_ZIP_AARCH64" ]] || { log "ERROR: Missing source-built AArch64 bootstrap: $RAF_REAL_BOOTSTRAP_ZIP_AARCH64"; exit 1; }
    [[ -f "$RAF_REAL_BOOTSTRAP_MANIFEST" ]] || { log "ERROR: Missing source-built bootstrap manifest: $RAF_REAL_BOOTSTRAP_MANIFEST"; exit 1; }

    # Validate the complete pair before changing any embedded archive.
    python3 scripts/import_rafcodephi_real_bootstrap.py \
      --arch arm \
      --zip "$RAF_REAL_BOOTSTRAP_ZIP_ARM" \
      --manifest "$RAF_REAL_BOOTSTRAP_MANIFEST" \
      --validate-only >&2
    python3 scripts/import_rafcodephi_real_bootstrap.py \
      --arch aarch64 \
      --zip "$RAF_REAL_BOOTSTRAP_ZIP_AARCH64" \
      --manifest "$RAF_REAL_BOOTSTRAP_MANIFEST" \
      --validate-only >&2

    # A clean checkout has no generated rewritten archives. After both source
    # inputs pass, materialize the compatibility matrix, then atomically replace
    # the two ARM slots with the validated source-built pair.
    bash scripts/build_bootstrap_profile.sh >&2

    python3 scripts/import_rafcodephi_real_bootstrap.py \
      --arch arm \
      --zip "$RAF_REAL_BOOTSTRAP_ZIP_ARM" \
      --manifest "$RAF_REAL_BOOTSTRAP_MANIFEST" \
      --dest app/src/main/cpp/rewritten-bootstrap-arm.zip \
      --receipt build/reports/rafcodephi-real-bootstrap-import-arm.json >&2
    python3 scripts/import_rafcodephi_real_bootstrap.py \
      --arch aarch64 \
      --zip "$RAF_REAL_BOOTSTRAP_ZIP_AARCH64" \
      --manifest "$RAF_REAL_BOOTSTRAP_MANIFEST" \
      --dest app/src/main/cpp/rewritten-bootstrap-aarch64.zip \
      --receipt build/reports/rafcodephi-real-bootstrap-import-aarch64.json >&2

    SOURCE_BUILT_RECEIPT_MATRIX="build/reports/rafcodephi-real-bootstrap-import-matrix.json"
    python3 - "$RAF_REAL_BOOTSTRAP_MANIFEST" "$SOURCE_BUILT_RECEIPT_MATRIX" <<'PY'
from __future__ import annotations
import hashlib
import json
import sys
from pathlib import Path

manifest_path = Path(sys.argv[1])
matrix_path = Path(sys.argv[2])
receipts = {}
for arch in ("arm", "aarch64"):
    path = Path(f"build/reports/rafcodephi-real-bootstrap-import-{arch}.json")
    doc = json.loads(path.read_text(encoding="utf-8"))
    if doc.get("arch") != arch or doc.get("device_runtime_proof") != "TOKEN_VAZIO":
        raise SystemExit(f"invalid source-built receipt boundary for {arch}")
    receipts[arch] = {
        "path": str(path),
        "sha256": hashlib.sha256(path.read_bytes()).hexdigest(),
        "bootstrap_sha256": doc.get("sha256"),
        "package_repo_runtime_state": doc.get("package_repo_runtime_state"),
        "apt_update_guard": doc.get("apt_update_guard"),
    }
matrix = {
    "schema": "rafcodephi.real-bootstrap-import-matrix/v1",
    "structural_state": "PASS",
    "package_name": "com.termux.rafacodephi",
    "api_package": "com.termux.rafacodephi.api",
    "api_receiver_component": "com.termux.rafacodephi.api/com.termux.api.TermuxApiReceiver",
    "api_access_control": "SIGNATURE_PERMISSION_NO_SHARED_UID",
    "package_repo_runtime_state": "BLOCKED_CUSTOM_REPOSITORY_NOT_PUBLISHED",
    "apt_update_guard": "RAFCODEPHI_PACKAGE_REPOSITORY_NOT_PUBLISHED",
    "architectures": receipts,
    "source_manifest": str(manifest_path),
    "source_manifest_sha256": hashlib.sha256(manifest_path.read_bytes()).hexdigest(),
    "paired_architectures_complete": True,
    "claim_allowed_device_runtime": False,
    "device_runtime_proof": "TOKEN_VAZIO",
}
matrix_path.parent.mkdir(parents=True, exist_ok=True)
matrix_path.write_text(json.dumps(matrix, indent=2, sort_keys=True) + "\n", encoding="utf-8")
PY
    PROFILE_REQUIREMENT="required"
    log "Source-built real ARM/AArch64 bootstrap pair imported fail-closed"
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
  [[ -n "$SOURCE_BUILT_RECEIPT_MATRIX" && -s "$SOURCE_BUILT_RECEIPT_MATRIX" ]] || {
    log "ERROR: source-built-real import receipt matrix missing"
    exit 1
  }
  IMPORT_SHA256="$(sha256sum "$SOURCE_BUILT_RECEIPT_MATRIX" | awk '{print $1}')"
  [[ "$IMPORT_SHA256" =~ ^[0-9a-f]{64}$ ]] || { log "ERROR: Invalid source-built import receipt matrix SHA256"; exit 1; }
  log "Source-built import receipt matrix SHA256: $IMPORT_SHA256"
fi

BLAKE3_TARGET="$ROOT_DIR/build/python-deps"
if [[ -d "$BLAKE3_TARGET/blake3" ]]; then
  export PYTHONPATH="$BLAKE3_TARGET${PYTHONPATH:+:$PYTHONPATH}"
fi
if ! python3 -c 'import blake3' >/dev/null 2>&1; then
  log "Installing blake3 into writable build target: $BLAKE3_TARGET"
  mkdir -p "$BLAKE3_TARGET"
  python3 -m pip install --disable-pip-version-check --upgrade --target "$BLAKE3_TARGET" blake3 >&2
  export PYTHONPATH="$BLAKE3_TARGET${PYTHONPATH:+:$PYTHONPATH}"
  python3 -c 'import blake3' >/dev/null 2>&1 || {
    log "ERROR: blake3 remains unavailable after isolated install"
    exit 1
  }
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
