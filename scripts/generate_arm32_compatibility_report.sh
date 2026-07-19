#!/usr/bin/env bash
set -euo pipefail

ARM32_APK="${1:-}"
UNIVERSAL_APK="${2:-}"
REPORT_DIR="${3:-}"

if [[ -z "${ARM32_APK}" || -z "${UNIVERSAL_APK}" || -z "${REPORT_DIR}" ]]; then
  echo "usage: $0 <arm32-apk> <universal-apk> <report-directory>" >&2
  exit 64
fi

mkdir -p "${REPORT_DIR}"

bash scripts/collect_apk_badging.sh "${ARM32_APK}" "${REPORT_DIR}"
unzip -l "${ARM32_APK}" | tee "${REPORT_DIR}/native-libs.txt"
unzip -l "${UNIVERSAL_APK}" > "${REPORT_DIR}/universal-libs.txt"

lib_path="$(unzip -Z1 "${ARM32_APK}" | grep '^lib/armeabi-v7a/.*\.so$' | head -n1)"
if [[ -z "${lib_path}" ]]; then
  echo "ERROR: no armeabi-v7a shared library found" >&2
  exit 1
fi

unzip -p "${ARM32_APK}" "${lib_path}" > /tmp/rafcodephi-arm32-lib.so
readelf -h /tmp/rafcodephi-arm32-lib.so > "${REPORT_DIR}/readelf-arm32.txt"
readelf -A /tmp/rafcodephi-arm32-lib.so >> "${REPORT_DIR}/readelf-arm32.txt"

APKSIGNER="$(bash scripts/find_android_sdk_tool.sh apksigner)"
printf 'apksigner=%s\n' "${APKSIGNER}" > "${REPORT_DIR}/android-sdk-tools.txt"
"${APKSIGNER}" verify --verbose "${ARM32_APK}" 2>&1 \
  | tee "${REPORT_DIR}/apk-signature.txt"

test -s "${REPORT_DIR}/apk-badging.txt"
test -s "${REPORT_DIR}/native-libs.txt"
test -s "${REPORT_DIR}/universal-libs.txt"
test -s "${REPORT_DIR}/readelf-arm32.txt"
test -s "${REPORT_DIR}/apk-signature.txt"
