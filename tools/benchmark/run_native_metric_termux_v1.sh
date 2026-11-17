#!/usr/bin/env bash
# RAFAELIA Ω — on-device ARM32/AArch64 hosted measurement; source-only.
# No APT edits, network, root, device permissions, or repo publication.
set -euo pipefail

expected_prefix='/data/data/com.termux.rafacodephi/files/usr'
if [[ "${PREFIX:-}" != "$expected_prefix" && "${PREFIX:-}" != '/data/user/0/com.termux.rafacodephi/files/usr' ]]; then
  printf 'RAFAELIA_BENCH=BLOCKED_PREFIX observed=%s\n' "${PREFIX:-TOKEN_VAZIO}" >&2
  exit 2
fi
if ! command -v clang >/dev/null 2>&1; then
  echo 'RAFAELIA_BENCH=BLOCKED_NO_CLANG (install only via a verified prefix-matched signed repository)' >&2
  exit 3
fi
if [[ "${1:-10000000}" =~ ^[0-9]+$ && "${2:-7}" =~ ^[0-9]+$ ]]; then
  iterations="${1:-10000000}"
  rounds="${2:-7}"
else
  echo 'RAFAELIA_BENCH=BLOCKED_BAD_ARGUMENTS' >&2
  exit 64
fi
src_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source_file="$src_dir/raf_native_integer_sum_v1.c"
[[ -r "$source_file" ]] || { echo 'RAFAELIA_BENCH=BLOCKED_NO_SOURCE' >&2; exit 4; }
output_dir="${HOME}/.cache/rafcodephi-native-metric-v1"
mkdir -p "$output_dir"
binary="$output_dir/raf_native_integer_sum_v1"
clang -std=c11 -O3 -Wall -Wextra -Werror "$source_file" -o "$binary"
[[ -x "$binary" ]] || { echo 'RAFAELIA_BENCH=BLOCKED_BUILD' >&2; exit 5; }
receipt="$output_dir/run-$(date -u +%Y%m%dT%H%M%SZ).jsonl"
{
  printf 'RAFAELIA_BENCH=LOCAL_DEVICE_SCOPED\n'
  printf 'PREFIX=%s\nARCH=%s\n' "$PREFIX" "$(uname -m)"
  if command -v getprop >/dev/null 2>&1; then
    printf 'ANDROID_API=%s\n' "$(getprop ro.build.version.sdk)"
  fi
  printf 'SOURCE_SHA256='
  if command -v sha256sum >/dev/null 2>&1; then
    sha256sum "$source_file" | awk '{print $1}'
  else
    busybox sha256sum "$source_file" | busybox awk '{print $1}'
  fi
} > "$output_dir/environment.txt"
"$binary" "$iterations" "$rounds" | tee "$receipt"
if [[ ! -s "$receipt" ]]; then
  echo 'RAFAELIA_BENCH=FAIL_EMPTY_RESULT' >&2; exit 6
fi
printf 'RAFAELIA_BENCH=PASS_SCOPED_ON_DEVICE_CANDIDATE\n'
printf 'EVIDENCE_FILE=%s\n' "$receipt"
printf 'CLAIM_ALLOWED=false\n'
