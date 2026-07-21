#!/usr/bin/env sh
set -eu

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)"
OUT="${1:-artifacts/termux-runtime-evidence.json}"

if ! command -v python3 >/dev/null 2>&1; then
  printf '%s\n' \
    'BLOCKED: python3 is required to seal the v2 receipt without inventing a digest.' \
    >&2
  exit 2
fi

exec python3 "$ROOT/tools/collect_runtime_receipt_v2.py" "$OUT" \
  ${PRODUCER_COMMIT:+--producer-commit "$PRODUCER_COMMIT"} \
  ${APK_PATH:+--apk-path "$APK_PATH"} \
  ${TERMUX_PACKAGE_NAME:+--package-name "$TERMUX_PACKAGE_NAME"}
