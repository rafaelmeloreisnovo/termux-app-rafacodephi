#!/usr/bin/env bash
set -euo pipefail
PREFIX="${RAF_TERMUX_PREFIX:-/data/data/com.termux/files/usr}"
OUT_DIR="${1:-$PWD/out-termux-compat}"
mkdir -p "$OUT_DIR/bin" "$OUT_DIR/share/bootstrap_rafaelia" "$OUT_DIR/lib/bootstrap_rafaelia"
cat > "$OUT_DIR/bin/bootstrap-rafaelia-selftest" <<'SH'
#!/usr/bin/env sh
set -eu
if [ -x "$PREFIX/lib/bootstrap_rafaelia/raf_selftest" ]; then
  exec "$PREFIX/lib/bootstrap_rafaelia/raf_selftest"
fi
echo "raf_selftest not installed in $PREFIX/lib/bootstrap_rafaelia"
exit 1
SH
chmod +x "$OUT_DIR/bin/bootstrap-rafaelia-selftest"
cat > "$OUT_DIR/share/bootstrap_rafaelia/NOTICE.txt" <<'TXT'
bootstrap_rafaelia is experimental and auxiliary.
It does NOT replace TermuxInstaller bootstrap.zip flow.
It does NOT replace SYMLINKS.txt validation.
TXT
cp -f README.md "$OUT_DIR/share/bootstrap_rafaelia/README.md"
echo "Generated Termux-compatible auxiliary payload at: $OUT_DIR"
