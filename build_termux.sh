#!/usr/bin/env bash
set -euo pipefail

ARCH_RAW="$(uname -m)"
MODE="host"
ARCH="host"
EXTRA_CFLAGS=""

case "$ARCH_RAW" in
  aarch64)
    MODE="termux"
    ARCH="arm64"
    EXTRA_CFLAGS="-march=armv8-a"
    ;;
  armv7l|armv8l|arm*)
    MODE="termux"
    ARCH="arm32"
    EXTRA_CFLAGS="-march=armv7-a -mfloat-abi=softfp"
    ;;
  x86_64|amd64|i686|i386)
    MODE="host"
    ARCH="host"
    ;;
  *)
    MODE="host"
    ARCH="host"
    ;;
esac

echo "[build_termux] detected uname -m=${ARCH_RAW} mode=${MODE} arch=${ARCH}"

make clean

if [[ "$ARCH" == "host" ]]; then
  make CC=clang diagnose
else
  make CC=clang ARCH="$ARCH" EXTRA_CFLAGS="$EXTRA_CFLAGS" diagnose
fi

if [[ ! -x bootstrap_rafaelia/raf_selftest ]]; then
  echo "[build_termux] ERROR: binary bootstrap_rafaelia/raf_selftest not found"
  exit 1
fi

echo "[build_termux] running bootstrap_rafaelia/raf_selftest"
out="$(./bootstrap_rafaelia/raf_selftest)"
printf '%s\n' "$out"

if printf '%s\n' "$out" | grep -Eq 'SELFTEST total_fail[[:space:]]+0|ok=[0-9]+ fail=0'; then
  echo "SELFTEST total_fail 0"
  exit 0
fi

echo "[build_termux] SELFTEST failure"
exit 1
