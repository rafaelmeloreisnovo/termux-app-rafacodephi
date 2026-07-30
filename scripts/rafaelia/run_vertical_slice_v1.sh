#!/data/data/com.termux/files/usr/bin/bash
set -euo pipefail

MAPA_ROOT="${1:?usage: run_vertical_slice_v1.sh MAPA_CHECKOUT SOURCE_ROOT}"
SOURCE_ROOT="${2:?usage: run_vertical_slice_v1.sh MAPA_CHECKOUT SOURCE_ROOT}"
RUNNER="$MAPA_ROOT/scripts/run_vertical_slice_v1.py"
REGISTRY="$MAPA_ROOT/data/vertical_slice_v1/registry.json"
RECEIPT_DIR="$MAPA_ROOT/receipts/vertical_slice"
RECEIPT="$RECEIPT_DIR/RECEIPT-VSLICE-001.termux.json"

command -v python3 >/dev/null 2>&1 || { echo 'FAIL: python3 unavailable' >&2; exit 127; }
command -v sha256sum >/dev/null 2>&1 || { echo 'FAIL: sha256sum unavailable' >&2; exit 127; }
[ -f "$RUNNER" ] || { echo "FAIL: missing Mapa runner: $RUNNER" >&2; exit 2; }
[ -f "$REGISTRY" ] || { echo "FAIL: missing Mapa registry: $REGISTRY" >&2; exit 2; }
[ -d "$SOURCE_ROOT" ] || { echo "FAIL: source root is not a directory: $SOURCE_ROOT" >&2; exit 2; }

mkdir -p "$RECEIPT_DIR"
python3 "$RUNNER" \
  --source-root "$SOURCE_ROOT" \
  --repository-root "$MAPA_ROOT" \
  --runtime-class ANDROID_TERMUX_LOCAL \
  --receipt-out "$RECEIPT"
sha256sum "$RECEIPT" > "$RECEIPT.sha256"

printf 'MAPA_ROOT=%s\n' "$MAPA_ROOT"
printf 'SOURCE_ROOT=%s\n' "$SOURCE_ROOT"
printf 'RECEIPT=%s\n' "$RECEIPT"
printf 'RECEIPT_SHA256_FILE=%s\n' "$RECEIPT.sha256"
printf 'STATE=LOCAL_RECEIPT_CREATED_HUMAN_REVIEW_PENDING\n'
