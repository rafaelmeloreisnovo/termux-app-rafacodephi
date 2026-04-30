#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT_DIR"

MODE="--print-env"
if [[ ${1:-} == "--github-env" || ${1:-} == "--print-env" ]]; then
  MODE="$1"
elif [[ $# -gt 0 ]]; then
  echo "Usage: $0 [--print-env|--github-env]" >&2
  exit 1
fi

./scripts/ci_android_preflight.sh
./gradlew :app:downloadBootstraps --no-daemon

if ! python3 -c 'import blake3' >/dev/null 2>&1; then
  python3 -m pip install --user blake3
fi

readarray -t HASH_LINES < <(python3 - <<'PY'
from pathlib import Path
from blake3 import blake3
import re

base = Path('app/src/main/cpp')
mapping = {
    'TERMUX_BOOTSTRAP_BLAKE3_AARCH64': 'bootstrap-aarch64.zip',
    'TERMUX_BOOTSTRAP_BLAKE3_ARM': 'bootstrap-arm.zip',
    'TERMUX_BOOTSTRAP_BLAKE3_I686': 'bootstrap-i686.zip',
    'TERMUX_BOOTSTRAP_BLAKE3_X86_64': 'bootstrap-x86_64.zip',
}
for env_key, file_name in mapping.items():
    path = base / file_name
    if not path.is_file():
        raise SystemExit(f"Missing bootstrap archive: {path}")
    digest = blake3(path.read_bytes()).hexdigest()
    if not re.fullmatch(r'[0-9a-f]{64}', digest):
        raise SystemExit(f"Invalid BLAKE3 for {path}: {digest}")
    print(f"{env_key}={digest}")
PY
)

if [[ ${#HASH_LINES[@]} -ne 4 ]]; then
  echo "Expected 4 bootstrap hash lines, got ${#HASH_LINES[@]}" >&2
  exit 1
fi

if [[ "$MODE" == "--github-env" ]]; then
  : "${GITHUB_ENV:?GITHUB_ENV must be set for --github-env mode}"
  for line in "${HASH_LINES[@]}"; do
    echo "$line" >> "$GITHUB_ENV"
  done
else
  for line in "${HASH_LINES[@]}"; do
    echo "export $line"
  done
fi
