#!/system/bin/sh
set -eu

SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
SRC="$ROOT_DIR/bootstrap/proot_freestanding.c"
OUT_DIR="${RAF_FS_BUILD_DIR:-$ROOT_DIR/build/freestanding-runtime}"
PREFIX_DIR="${PREFIX:-/data/data/com.termux.rafacodephi/files/usr}"
INSTALL_DIR="$PREFIX_DIR/libexec"
ARCH="${1:-$(uname -m)}"
EXTRA=""

case "$ARCH" in
  aarch64|arm64|arm64-v8a)
    TARGET="aarch64-linux-android21"
    NAME="aarch64"
    ;;
  armv7l|armv7|armeabi-v7a|arm)
    TARGET="armv7a-linux-androideabi21"
    NAME="armv7"
    EXTRA="-marm"
    ;;
  *)
    printf 'TOKEN_VAZIO: unsupported local ABI for freestanding gate: %s\n' "$ARCH" >&2
    exit 64
    ;;
esac

mkdir -p "$OUT_DIR" "$INSTALL_DIR"
OUT="$OUT_DIR/rafproot-fs-$NAME"

# EXTRA is intentionally one trusted compile flag selected by the ABI case.
# shellcheck disable=SC2086
clang --target="$TARGET" $EXTRA \
  -std=c11 -Wall -Wextra -Werror \
  -ffreestanding -fno-builtin -fno-stack-protector -fomit-frame-pointer \
  -nostdlib -static \
  -Wl,-no-pie,-e,_start,--gc-sections \
  "$SRC" -o "$OUT"

if command -v readelf >/dev/null 2>&1; then
  if readelf -l "$OUT" | grep -q 'INTERP'; then
    printf '%s\n' 'freestanding gate rejected: dynamic interpreter present' >&2
    exit 70
  fi
  if readelf -d "$OUT" 2>/dev/null | grep -q 'NEEDED'; then
    printf '%s\n' 'freestanding gate rejected: dynamic dependency present' >&2
    exit 71
  fi
fi

cp "$OUT" "$INSTALL_DIR/rafproot-fs"
chmod 700 "$INSTALL_DIR/rafproot-fs"

SHA="TOKEN_VAZIO"
if command -v sha256sum >/dev/null 2>&1; then
  SHA="$(sha256sum "$INSTALL_DIR/rafproot-fs" | awk '{print $1}')"
fi

cat > "$OUT_DIR/receipt-$NAME.json" <<EOF
{
  "schema": "raf.freestanding-runtime-gate.local.v1",
  "architecture": "$NAME",
  "target": "$TARGET",
  "installed_path": "$INSTALL_DIR/rafproot-fs",
  "sha256": "$SHA",
  "build_state": "BUILD_PROVEN",
  "device_runtime_state": "TOKEN_VAZIO",
  "claim_allowed": false
}
EOF

printf 'BUILD_PROVEN: %s\n' "$INSTALL_DIR/rafproot-fs"
printf 'receipt: %s\n' "$OUT_DIR/receipt-$NAME.json"
printf 'next: %s --probe\n' "$INSTALL_DIR/rafproot-fs"
