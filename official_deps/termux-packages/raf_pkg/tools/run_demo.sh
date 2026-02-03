#!/usr/bin/env bash
set -euo pipefail
HERE=$(cd "$(dirname "$0")/.." && pwd)
"$HERE/tools/build.sh"
echo
"$HERE/out/rafpkg" scan --out "$HERE/out" || true
"$HERE/out/rafpkg" audit || true
