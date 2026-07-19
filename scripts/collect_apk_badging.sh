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

SDK_ROOT="${ANDROID_SDK_ROOT:-${ANDROID_HOME:-}}"

find_sdk_tool() {
  local name="$1"
  local resolved=""
  if command -v "${name}" >/dev/null 2>&1; then
    command -v "${name}"
    return 0
  fi
  if [[ -n "${SDK_ROOT}" && -d "${SDK_ROOT}" ]]; then
    case "${name}" in
      aapt|aapt2)
        resolved="$(find "${SDK_ROOT}/build-tools" -mindepth 2 -maxdepth 2 -type f -name "${name}" 2>/dev/null | sort -V | tail -n1)"
        ;;
      apkanalyzer)
        resolved="$(find "${SDK_ROOT}/cmdline-tools" -type f -path '*/bin/apkanalyzer' 2>/dev/null | sort -V | tail -n1)"
        ;;
    esac
  fi
  if [[ -n "${resolved}" ]]; then
    printf '%s\n' "${resolved}"
    return 0
  fi
  return 1
}

capture_badging() {
  local tool_name="$1"
  shift
  local tmp_out tmp_err
  tmp_out="$(mktemp)"
  tmp_err="$(mktemp)"
  if "$@" >"${tmp_out}" 2>"${tmp_err}" && [[ -s "${tmp_out}" ]]; then
    cat "${tmp_out}" > "${BADGING}"
    printf 'source=%s\ncommand=%q\n' "${tool_name}" "$1" > "${SOURCE}"
    rm -f "${tmp_out}" "${tmp_err}"
    return 0
  fi
  {
    printf '[%s]\n' "${tool_name}"
    printf 'command=%q\n' "$1"
    cat "${tmp_err}"
    printf '\n'
  } >> "${ERRORS}"
  rm -f "${tmp_out}" "${tmp_err}"
  return 1
}

AAPT="$(find_sdk_tool aapt || true)"
AAPT2="$(find_sdk_tool aapt2 || true)"
APKANALYZER="$(find_sdk_tool apkanalyzer || true)"

if [[ -n "${AAPT}" ]] && capture_badging "aapt dump badging" "${AAPT}" dump badging "${APK}"; then
  :
elif [[ -n "${AAPT2}" ]] && capture_badging "aapt2 dump badging" "${AAPT2}" dump badging "${APK}"; then
  :
else
  if [[ -z "${APKANALYZER}" ]]; then
    {
      printf '[tool-resolution]\n'
      printf 'ANDROID_SDK_ROOT=%s\n' "${ANDROID_SDK_ROOT:-}"
      printf 'ANDROID_HOME=%s\n' "${ANDROID_HOME:-}"
      printf 'aapt=%s\n' "${AAPT:-NOT_FOUND}"
      printf 'aapt2=%s\n' "${AAPT2:-NOT_FOUND}"
      printf 'apkanalyzer=NOT_FOUND\n'
    } >> "${ERRORS}"
    echo "ERROR: no usable APK metadata tool found in PATH or Android SDK" >&2
    cat "${ERRORS}" >&2
    exit 1
  fi

  package_name="$(${APKANALYZER} manifest application-id "${APK}" 2>>"${ERRORS}" || true)"
  min_sdk="$(${APKANALYZER} manifest min-sdk "${APK}" 2>>"${ERRORS}" || true)"
  target_sdk="$(${APKANALYZER} manifest target-sdk "${APK}" 2>>"${ERRORS}" || true)"
  package_name="$(tr -d '\r\n' <<<"${package_name}")"
  min_sdk="$(tr -d '\r\n' <<<"${min_sdk}")"
  target_sdk="$(tr -d '\r\n' <<<"${target_sdk}")"
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
      printf "native-code: '%s'\n" "${native_abis}"
    fi
  } > "${BADGING}"
  {
    printf 'source=apkanalyzer+zip-inventory\n'
    printf 'command=%s\n' "${APKANALYZER}"
  } > "${SOURCE}"
fi

if [[ ! -s "${BADGING}" ]]; then
  echo "ERROR: metadata collector produced an empty badging report" >&2
  cat "${ERRORS}" >&2
  exit 1
fi

cat "${SOURCE}"
cat "${BADGING}"
