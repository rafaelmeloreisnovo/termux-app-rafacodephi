#!/usr/bin/env bash
set -euo pipefail

# Sincroniza lista de pacotes do upstream termux/termux-packages
# e regenera rafaelia/src/main/cpp/raf_termux_packages.c sem Python.

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
OUT_C="$ROOT_DIR/rafaelia/src/main/cpp/raf_termux_packages.c"
TEMPLATE_C="$ROOT_DIR/scripts/templates/raf_termux_packages.c.tpl"
TOOL_SRC="$ROOT_DIR/rafaelia/src/main/cpp/tools/raf_termux_pkg_tool.c"
TMP_DIR="$(mktemp -d)"
trap 'rm -rf "$TMP_DIR"' EXIT

UPSTREAM_URL="${1:-https://github.com/termux/termux-packages.git}"
TOOL_BIN="$TMP_DIR/raf_termux_pkg_tool"

printf '[sync] compile tool %s\n' "$TOOL_SRC"
cc -std=c99 -O2 -Wall -Wextra -Werror "$TOOL_SRC" -o "$TOOL_BIN"

printf '[sync] clone %s\n' "$UPSTREAM_URL"
git clone --depth 1 "$UPSTREAM_URL" "$TMP_DIR/termux-packages" >/dev/null 2>&1

printf '[sync] gerar tabela deterministica via template\n'
"$TOOL_BIN" gen-packages-c "$TMP_DIR/termux-packages" "$TEMPLATE_C" "$OUT_C"

printf '[sync] pronto: %s\n' "$OUT_C"
