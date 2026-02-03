#!/data/data/com.termux/files/usr/bin/bash
# RAFAELIA :: demo (run inside termux-packages)
set -e

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT/.." # termux-packages root

"$ROOT/tools/build.sh" > /dev/null

echo "== rafpkg audit =="
"$ROOT/out/rafpkg" audit --arch "${TERMUX_ARCH:-aarch64}" | head -n 5

echo
echo "== rafpkg plan (first 25) =="
"$ROOT/out/rafpkg" plan --arch "${TERMUX_ARCH:-aarch64}" | head -n 25
