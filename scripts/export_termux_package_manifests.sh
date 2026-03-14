#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
OUT_DIR="$ROOT_DIR/rafaelia/termux-packages-manifests"
TOOL_SRC="$ROOT_DIR/rafaelia/src/main/cpp/tools/raf_termux_pkg_tool.c"
TMP_DIR="$(mktemp -d)"
trap 'rm -rf "$TMP_DIR"' EXIT

UPSTREAM_URL="${1:-https://github.com/termux/termux-packages.git}"
LIMIT="${2:-700}"
TOOL_BIN="$TMP_DIR/raf_termux_pkg_tool"

printf '[export] compile tool %s\n' "$TOOL_SRC"
cc -std=c99 -O2 -Wall -Wextra -Werror "$TOOL_SRC" -o "$TOOL_BIN"

printf '[export] clone %s\n' "$UPSTREAM_URL"
git clone --depth 1 "$UPSTREAM_URL" "$TMP_DIR/termux-packages" >/dev/null 2>&1

printf '[export] gerar manifests deterministicos (limit=%s)\n' "$LIMIT"
"$TOOL_BIN" export-manifests "$TMP_DIR/termux-packages" "$OUT_DIR" "$LIMIT" "$UPSTREAM_URL"

printf '[export] pronto: %s\n' "$OUT_DIR"
