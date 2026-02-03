#!/usr/bin/env bash
# RAFAELIA :: helper to build & run rafpkg
set -e

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
RAFPKG_DIR="$ROOT/raf_pkg"

if [ ! -x "$RAFPKG_DIR/out/rafpkg" ]; then
  (cd "$RAFPKG_DIR" && bash tools/build.sh)
fi

exec "$RAFPKG_DIR/out/rafpkg" "$@"
