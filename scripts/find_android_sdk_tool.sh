#!/usr/bin/env bash
set -euo pipefail

TOOL="${1:-}"
if [[ -z "${TOOL}" ]]; then
  echo "usage: $0 <tool-name>" >&2
  exit 64
fi

if command -v "${TOOL}" >/dev/null 2>&1; then
  command -v "${TOOL}"
  exit 0
fi

SDK_ROOT="${ANDROID_SDK_ROOT:-${ANDROID_HOME:-}}"
if [[ -z "${SDK_ROOT}" || ! -d "${SDK_ROOT}" ]]; then
  echo "ERROR: Android SDK root is unavailable" >&2
  exit 1
fi

resolved=""
case "${TOOL}" in
  aapt|aapt2|apksigner|zipalign)
    resolved="$(find "${SDK_ROOT}/build-tools" -mindepth 2 -maxdepth 2 -type f -name "${TOOL}" 2>/dev/null | sort -V | tail -n1)"
    ;;
  apkanalyzer)
    resolved="$(find "${SDK_ROOT}/cmdline-tools" -type f -path '*/bin/apkanalyzer' 2>/dev/null | sort -V | tail -n1)"
    ;;
  *)
    resolved="$(find "${SDK_ROOT}" -type f -name "${TOOL}" 2>/dev/null | sort -V | tail -n1)"
    ;;
esac

if [[ -z "${resolved}" || ! -x "${resolved}" ]]; then
  echo "ERROR: Android SDK tool not found: ${TOOL}" >&2
  exit 1
fi

printf '%s\n' "${resolved}"
