#!/usr/bin/env bash
set -euo pipefail

APK="${1:-}"
REPORT="${2:-}"

if [[ -z "${APK}" || -z "${REPORT}" ]]; then
  echo "usage: $0 <loader.apk> <report-file>" >&2
  exit 64
fi
if [[ ! -f "${APK}" ]]; then
  echo "ERROR: loader APK not found: ${APK}" >&2
  exit 1
fi

mkdir -p "$(dirname "${REPORT}")"
: > "${REPORT}"

APKANALYZER="$(bash scripts/find_android_sdk_tool.sh apkanalyzer)"
APKSIGNER="$(bash scripts/find_android_sdk_tool.sh apksigner)"

package_name="$(${APKANALYZER} manifest application-id "${APK}" | tr -d '\r\n')"
min_sdk="$(${APKANALYZER} manifest min-sdk "${APK}" | tr -d '\r\n')"
target_sdk="$(${APKANALYZER} manifest target-sdk "${APK}" | tr -d '\r\n')"

if [[ "${package_name}" != "com.termux.rafacodephi.loader" ]]; then
  echo "ERROR: unexpected loader package: ${package_name}" >&2
  exit 1
fi
if [[ "${min_sdk}" != "21" ]]; then
  echo "ERROR: unexpected loader minSdk: ${min_sdk}" >&2
  exit 1
fi
if [[ "${target_sdk}" != "28" ]]; then
  echo "ERROR: unexpected loader targetSdk: ${target_sdk}" >&2
  exit 1
fi

if unzip -Z1 "${APK}" | grep -Eq '^classes([0-9]*)?\.dex$'; then
  echo "ERROR: loader stub must not contain executable DEX code" >&2
  exit 1
fi

"${APKSIGNER}" verify --verbose "${APK}" > /tmp/loader-apksigner.txt 2>&1
sha256="$(sha256sum "${APK}" | awk '{print $1}')"

{
  printf 'state=STUB_NO_BOOTSTRAP_PAYLOAD\n'
  printf 'package=%s\n' "${package_name}"
  printf 'min_sdk=%s\n' "${min_sdk}"
  printf 'target_sdk=%s\n' "${target_sdk}"
  printf 'has_dex=false\n'
  printf 'signed=true\n'
  printf 'sha256=%s\n' "${sha256}"
  printf 'size_bytes=%s\n' "$(stat -c '%s' "${APK}")"
  printf 'apkanalyzer=%s\n' "${APKANALYZER}"
  printf 'apksigner=%s\n' "${APKSIGNER}"
  printf '\n[apksigner]\n'
  cat /tmp/loader-apksigner.txt
} | tee "${REPORT}"
