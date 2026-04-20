#!/usr/bin/env bash
set -euo pipefail

GRADLE_PROPERTIES_FILE="${1:-gradle.properties}"

if [[ ! -f "$GRADLE_PROPERTIES_FILE" ]]; then
  echo "❌ Missing file: $GRADLE_PROPERTIES_FILE"
  exit 1
fi

read_prop() {
  local key="$1"
  grep -E "^${key}=" "$GRADLE_PROPERTIES_FILE" | head -n1 | cut -d= -f2- | tr -d '[:space:]' || true
}

COMPILE_SDK_VERSION="$(read_prop compileSdkVersion)"
TARGET_SDK_VERSION="$(read_prop targetSdkVersion)"
NDK_VERSION="$(read_prop ndkVersion)"
BUILD_TOOLS_VERSION="$(read_prop buildToolsVersion)"

if [[ -z "$COMPILE_SDK_VERSION" || -z "$TARGET_SDK_VERSION" || -z "$NDK_VERSION" ]]; then
  echo "❌ Missing required Android toolchain keys in $GRADLE_PROPERTIES_FILE"
  echo "Required: compileSdkVersion, targetSdkVersion, ndkVersion"
  exit 1
fi

if [[ -z "$BUILD_TOOLS_VERSION" ]]; then
  BUILD_TOOLS_VERSION="${COMPILE_SDK_VERSION}.0.0"
fi

echo "📦 Android toolchain source of truth: $GRADLE_PROPERTIES_FILE"
echo "compileSdkVersion=$COMPILE_SDK_VERSION"
echo "targetSdkVersion=$TARGET_SDK_VERSION"
echo "ndkVersion=$NDK_VERSION"
echo "buildToolsVersion=$BUILD_TOOLS_VERSION"

if ! command -v sdkmanager >/dev/null 2>&1; then
  echo "❌ sdkmanager not found in PATH. Ensure android-actions/setup-android has run before this script."
  exit 1
fi

# Keep `pipefail` globally but avoid false failure from `yes` receiving SIGPIPE.
set +e
yes | sdkmanager --licenses > /dev/null
sdkmanager_license_status=${PIPESTATUS[1]}
set -e
if [[ "$sdkmanager_license_status" -ne 0 ]]; then
  echo "❌ Failed to accept Android SDK licenses (sdkmanager exit code: $sdkmanager_license_status)."
  exit "$sdkmanager_license_status"
fi

sdkmanager \
  "platform-tools" \
  "platforms;android-${COMPILE_SDK_VERSION}" \
  "build-tools;${BUILD_TOOLS_VERSION}" \
  "ndk;${NDK_VERSION}" \
  "cmake;3.22.1"
