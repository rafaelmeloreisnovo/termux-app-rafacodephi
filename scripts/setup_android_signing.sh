#!/usr/bin/env bash
set -euo pipefail

if [[ "${GITHUB_EVENT_NAME:-}" == "workflow_dispatch" && "${INPUT_BUILD_TYPE:-}" != "" && "${INPUT_BUILD_TYPE}" != "release" ]]; then
  echo "ℹ️ Build type is '${INPUT_BUILD_TYPE}', skipping release signing setup."
  exit 0
fi

if [[ -z "${ANDROID_KEYSTORE_BASE64:-}" ]]; then
  echo "ℹ️ ANDROID_KEYSTORE_BASE64 not configured; release build will remain unsigned."
  exit 0
fi

required_vars=(
  ANDROID_KEYSTORE_PASSWORD
  ANDROID_KEY_ALIAS
  ANDROID_KEY_PASSWORD
)

for var_name in "${required_vars[@]}"; do
  if [[ -z "${!var_name:-}" ]]; then
    echo "❌ ${var_name} is required when ANDROID_KEYSTORE_BASE64 is set."
    exit 1
  fi
done

keystore_dir="${RUNNER_TEMP:-$PWD/.tmp}"
mkdir -p "$keystore_dir"
keystore_path="${keystore_dir}/termux-release.jks"

echo "$ANDROID_KEYSTORE_BASE64" | base64 --decode > "$keystore_path"

if [[ ! -s "$keystore_path" ]]; then
  echo "❌ Decoded keystore is empty: $keystore_path"
  exit 1
fi

echo "✅ Release keystore decoded successfully at $keystore_path"

if [[ -n "${GITHUB_ENV:-}" ]]; then
  {
    echo "TERMUX_ENABLE_RELEASE_SIGNING=true"
    echo "TERMUX_RELEASE_KEYSTORE_FILE=$keystore_path"
    echo "TERMUX_RELEASE_KEYSTORE_PASSWORD=$ANDROID_KEYSTORE_PASSWORD"
    echo "TERMUX_RELEASE_KEY_ALIAS=$ANDROID_KEY_ALIAS"
    echo "TERMUX_RELEASE_KEY_PASSWORD=$ANDROID_KEY_PASSWORD"
  } >> "$GITHUB_ENV"
fi
