#!/usr/bin/env bash
set -euo pipefail
CORE="$(cd "$(dirname "$0")" && pwd)/gdrive-sync-core.py"
command -v python3 >/dev/null 2>&1 || { echo 'ERROR: python3 is required by the fail-closed sync core' >&2; exit 127; }
exec python3 "$CORE" "$@"
