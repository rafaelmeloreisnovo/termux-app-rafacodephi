#!/data/data/com.termux/files/usr/bin/bash
# RAFAELIA :: build rafpkg (hosted)
set -e

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
OUT="$ROOT/out"
mkdir -p "$OUT"

CC=${CC:-clang}
CFLAGS="${CFLAGS:-}-O2 -std=c11 -Wall -Wextra -Wno-unused-parameter"

$CC $CFLAGS \
  "$ROOT/src/rafpkg_util.c" \
  "$ROOT/src/rafpkg_map.c" \
  "$ROOT/src/rafpkg_db.c" \
  "$ROOT/src/rafpkg_parse.c" \
  "$ROOT/src/rafpkg_scan.c" \
  "$ROOT/src/rafpkg_graph.c" \
  "$ROOT/src/rafpkg_json.c" \
  "$ROOT/src/rafpkg_cpu_id.c" \
  "$ROOT/src/rafpkg_fileaudit.c" \
  "$ROOT/src/rafpkg_store.c" \
  "$ROOT/src/rafpkg_main.c" \
  -o "$OUT/rafpkg"

echo "OK :: built $OUT/rafpkg"
