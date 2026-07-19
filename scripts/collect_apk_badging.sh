#!/usr/bin/env bash
set -euo pipefail

APK="${1:-}"
REPORT_DIR="${2:-}"

if [[ -z "${APK}" || -z "${REPORT_DIR}" ]]; then
  echo "usage: $0 <apk> <report-directory>" >&2
  exit 64
fi
if [[ ! -f "${APK}" ]]; then
  echo "ERROR: APK not found: ${APK}" >&2
  exit 1
fi

mkdir -p "${REPORT_DIR}"
BADGING="${REPORT_DIR}/apk-badging.txt"
SOURCE="${REPORT_DIR}/apk-metadata-source.txt"
ERRORS="${REPORT_DIR}/apk-metadata-errors.txt"
: > "${BADGING}"
: > "${SOURCE}"
: > "${ERRORS}"

capture_badging() {
  local tool="$1"
  shift
  local tmp_out tmp_err
  tmp_out="$(mktemp)"
  tmp_err="$(mktemp)"
  if "$@" >"${tmp_out}" 2>"${tmp_err}" && [[ -s "${tmp_out}" ]]; then
    cat "${tmp_out}" > "${BADGING}"
    printf 'source=%s\n' "${tool}" > "${SOURCE}"
    rm -f "${tmp_out}" "${tmp_err}"
    return 0
  fi
  {
    printf '[%s]\n' "${tool}"
    cat "${tmp_err}"
    printf '\n'
  } >> "${ERRORS}"
  rm -f "${tmp_out}" "${tmp_err}"
  return 1
}

if command -v aapt >/dev/null 2>&1 && capture_badging "aapt dump badging" aapt dump badging "${APK}"; then
  :
elif command -v aapt2 >/dev/null 2>&1 && capture_badging "aapt2 dump badging" aapt2 dump badging "${APK}"; then
  :
else
  if ! command -v apkanalyzer >/dev/null 2>&1; then
    echo "ERROR: aapt/aapt2 produced no badging and apkanalyzer is unavailable" >&2
    cat "${ERRORS}" >&2
    exit 1
  fi

  package_name="$(apkanalyzer manifest application-id "${APK}" 2>>"${ERRORS}" | tr -d '\r\n')"
  min_sdk="$(apkanalyzer manifest min-sdk "${APK}" 2>>"${ERRORS}" | tr -d '\r\n')"
  target_sdk="$(apkanalyzer manifest target-sdk "${APK}" 2>>"${ERRORS}" | tr -d '\r\n')"
  native_abis="$(unzip -Z1 "${APK}" | sed -n 's#^lib/\([^/]*\)/.*\.so$#\1#p' | sort -u | paste -sd' ' -)"

  if [[ -z "${package_name}" || -z "${min_sdk}" || -z "${target_sdk}" ]]; then
    echo "ERROR: unable to recover manifest metadata with apkanalyzer" >&2
    cat "${ERRORS}" >&2
    exit 1
  fi

  {
    printf "package: name='%s'\n" "${package_name}"
    printf "sdkVersion:'%s'\n" "${min_sdk}"
    printf "targetSdkVersion:'%s'\n" "${target_sdk}"
    if [[ -n "${native_abis}" ]]; then
      printf "native-code: '%s'\n" "${native_abis// /' ' '}'"
    fi
  } > "${BADGING}"
  printf 'source=apkanalyzer+zip-inventory\n' > "${SOURCE}"
fi

if [[ ! -s "${BADGING}" ]]; then
  echo "ERROR: metadata collector produced an empty badging report" >&2
  cat "${ERRORS}" >&2
  exit 1
fi

cat "${SOURCE}"
cat "${BADGING}"
