#!/usr/bin/env bash
set -u

REPORT_DIR="${1:-}"
if [[ -z "${REPORT_DIR}" ]]; then
  echo "usage: $0 <compatibility-report-directory>" >&2
  exit 64
fi

BADGING="${REPORT_DIR}/apk-badging.txt"
NATIVE_LIBS="${REPORT_DIR}/native-libs.txt"
UNIVERSAL_LIBS="${REPORT_DIR}/universal-libs.txt"
RESULTS="${REPORT_DIR}/gate-results.txt"

mkdir -p "${REPORT_DIR}"
: > "${RESULTS}"
failures=0

record() {
  printf '%s\n' "$1" | tee -a "${RESULTS}"
}

require_file() {
  local name="$1"
  local file="$2"
  if [[ -s "${file}" ]]; then
    record "PASS ${name}: ${file}"
    return 0
  fi
  record "FAIL ${name}: missing-or-empty ${file}"
  failures=$((failures + 1))
  return 1
}

require_fixed() {
  local name="$1"
  local expected="$2"
  local file="$3"
  if [[ -s "${file}" ]] && grep -Fq -- "${expected}" "${file}"; then
    record "PASS ${name}: ${expected}"
    return 0
  fi
  record "FAIL ${name}: expected fixed text not found: ${expected}"
  failures=$((failures + 1))
  return 1
}

require_regex() {
  local name="$1"
  local expected="$2"
  local file="$3"
  if [[ -s "${file}" ]] && grep -Eq -- "${expected}" "${file}"; then
    record "PASS ${name}: /${expected}/"
    return 0
  fi
  record "FAIL ${name}: expected regex not found: /${expected}/"
  failures=$((failures + 1))
  return 1
}

require_file "badging-report" "${BADGING}"
require_file "arm32-native-library-list" "${NATIVE_LIBS}"
require_file "universal-native-library-list" "${UNIVERSAL_LIBS}"

require_fixed "application-id" "package: name='com.termux.rafacodephi'" "${BADGING}"
require_fixed "minimum-sdk" "sdkVersion:'21'" "${BADGING}"
require_fixed "target-sdk" "targetSdkVersion:'28'" "${BADGING}"
require_regex "aapt-native-code-arm32" "native-code:.*armeabi-v7a" "${BADGING}"
require_fixed "split-apk-contains-arm32-lib" "lib/armeabi-v7a/" "${NATIVE_LIBS}"
require_fixed "universal-apk-contains-arm32-lib" "lib/armeabi-v7a/" "${UNIVERSAL_LIBS}"

if (( failures > 0 )); then
  record "SUMMARY FAIL failures=${failures}"
  exit 1
fi

record "SUMMARY PASS failures=0"
