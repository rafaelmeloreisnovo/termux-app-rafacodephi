#!/usr/bin/env bash
set -euo pipefail
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT_DIR"
cc -O2 -std=c11 -Wall -Wextra -Werror scripts/bootstrap_zip_builder.c -o /tmp/bootstrap_zip_builder
mkdir -p app/src/main/cpp
/tmp/bootstrap_zip_builder app/src/main/cpp/bootstrap-aarch64.zip aarch64
/tmp/bootstrap_zip_builder app/src/main/cpp/bootstrap-arm.zip arm
/tmp/bootstrap_zip_builder app/src/main/cpp/bootstrap-i686.zip i686
/tmp/bootstrap_zip_builder app/src/main/cpp/bootstrap-x86_64.zip x86_64
echo "developer bootstraps generated"
