#!/usr/bin/env bash
set -u

APK="${1:-}"
REPORT="${2:-}"

if [[ -z "${APK}" || -z "${REPORT}" ]]; then
  echo "usage: $0 <loader.apk> <report-file>" >&2
  exit 64
fi

mkdir -p "$(dirname "${REPORT}")"
: > "${REPORT}"

failures=0
record() {
  printf '%s\n' "$1" | tee -a "${REPORT}"
}
fail() {
  record "FAIL $1"
  failures=$((failures + 1))
}
pass() {
  record "PASS $1"
}

if [[ ! -f "${APK}" ]]; then
  fail "loader APK not found: ${APK}"
  record "SUMMARY FAIL failures=${failures}"
  exit 1
fi

APKANALYZER="$(bash scripts/find_android_sdk_tool.sh apkanalyzer 2>>"${REPORT}" || true)"
APKSIGNER="$(bash scripts/find_android_sdk_tool.sh apksigner 2>>"${REPORT}" || true)"
AAPT="$(bash scripts/find_android_sdk_tool.sh aapt 2>>"${REPORT}" || true)"

record "state=STUB_NO_BOOTSTRAP_PAYLOAD"
record "apk=${APK}"
record "size_bytes=$(stat -c '%s' "${APK}")"
record "sha256=$(sha256sum "${APK}" | awk '{print $1}')"
record "apkanalyzer=${APKANALYZER:-NOT_FOUND}"
record "apksigner=${APKSIGNER:-NOT_FOUND}"
record "aapt=${AAPT:-NOT_FOUND}"

package_name=""
min_sdk=""
target_sdk=""
if [[ -n "${APKANALYZER}" ]]; then
  package_name="$(${APKANALYZER} manifest application-id "${APK}" 2>>"${REPORT}" || true)"
  min_sdk="$(${APKANALYZER} manifest min-sdk "${APK}" 2>>"${REPORT}" || true)"
  target_sdk="$(${APKANALYZER} manifest target-sdk "${APK}" 2>>"${REPORT}" || true)"
  package_name="$(tr -d '\r\n' <<<"${package_name}")"
  min_sdk="$(tr -d '\r\n' <<<"${min_sdk}")"
  target_sdk="$(tr -d '\r\n' <<<"${target_sdk}")"
fi

if [[ -z "${package_name}" && -n "${AAPT}" ]]; then
  badging="$(${AAPT} dump badging "${APK}" 2>>"${REPORT}" || true)"
  package_name="$(sed -n "s/^package: name='\([^']*\)'.*/\1/p" <<<"${badging}" | head -n1)"
  min_sdk="$(sed -n "s/^sdkVersion:'\([^']*\)'.*/\1/p" <<<"${badging}" | head -n1)"
  target_sdk="$(sed -n "s/^targetSdkVersion:'\([^']*\)'.*/\1/p" <<<"${badging}" | head -n1)"
  printf '%s\n' "${badging}" > "$(dirname "${REPORT}")/loader-badging.txt"
fi

record "package=${package_name:-UNRESOLVED}"
record "min_sdk=${min_sdk:-UNRESOLVED}"
record "target_sdk=${target_sdk:-UNRESOLVED}"

if [[ "${package_name}" == "com.termux.rafacodephi.loader" ]]; then
  pass "package"
else
  fail "package expected=com.termux.rafacodephi.loader actual=${package_name:-UNRESOLVED}"
fi

if [[ "${min_sdk}" == "21" ]]; then
  pass "min_sdk"
else
  fail "min_sdk expected=21 actual=${min_sdk:-UNRESOLVED}"
fi

if [[ "${target_sdk}" == "28" ]]; then
  pass "target_sdk"
else
  fail "target_sdk expected=28 actual=${target_sdk:-UNRESOLVED}"
fi

if unzip -Z1 "${APK}" | grep -Eq '^classes([0-9]*)?\.dex$'; then
  record "has_dex=true"
  fail "loader stub must not contain executable DEX code"
else
  record "has_dex=false"
  pass "no_dex"
fi

if [[ -z "${APKSIGNER}" ]]; then
  record "signed=UNRESOLVED"
  fail "apksigner unavailable"
else
  signature_output="$(${APKSIGNER} verify --verbose "${APK}" 2>&1)"
  signature_status=$?
  printf '\n[apksigner]\n%s\n' "${signature_output}" | tee -a "${REPORT}"
  if [[ ${signature_status} -eq 0 ]]; then
    record "signed=true"
    pass "signature"
  else
    record "signed=false"
    fail "apksigner exit=${signature_status}"
  fi
fi

if (( failures > 0 )); then
  record "SUMMARY FAIL failures=${failures}"
  exit 1
fi

record "SUMMARY PASS failures=0"
