#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="${1:-.}"
LOCAL_PROPERTIES_PATH="${ROOT_DIR}/local.properties"

if [[ -f "${LOCAL_PROPERTIES_PATH}" ]] && grep -q '^sdk.dir=' "${LOCAL_PROPERTIES_PATH}"; then
  echo "✅ local.properties already has sdk.dir"
  exit 0
fi

candidate_sdk_dirs=()
if [[ -n "${ANDROID_HOME:-}" ]]; then
  candidate_sdk_dirs+=("${ANDROID_HOME}")
fi
if [[ -n "${ANDROID_SDK_ROOT:-}" ]]; then
  candidate_sdk_dirs+=("${ANDROID_SDK_ROOT}")
fi
candidate_sdk_dirs+=("${HOME}/Android/Sdk")
candidate_sdk_dirs+=("/usr/local/lib/android/sdk")
candidate_sdk_dirs+=("/opt/android-sdk")
candidate_sdk_dirs+=("/opt/android-sdk-linux")

sdk_dir=""
for candidate in "${candidate_sdk_dirs[@]}"; do
  if [[ -d "${candidate}" ]]; then
    sdk_dir="${candidate}"
    break
  fi
done

if [[ -z "${sdk_dir}" ]]; then
  echo "❌ Android SDK not found. Set ANDROID_HOME or ANDROID_SDK_ROOT, or install SDK to ${HOME}/Android/Sdk"
  exit 1
fi

escaped_sdk_dir="${sdk_dir//\\/\\\\}"
echo "sdk.dir=${escaped_sdk_dir}" > "${LOCAL_PROPERTIES_PATH}"
echo "✅ Generated ${LOCAL_PROPERTIES_PATH} with sdk.dir=${sdk_dir}"
