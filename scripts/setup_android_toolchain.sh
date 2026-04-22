#!/usr/bin/env bash
set -euo pipefail

GRADLE_PROPERTIES_FILE="${1:-gradle.properties}"
LOCAL_PROPERTIES_FILE="${2:-local.properties}"
SDK_ROOT_OVERRIDE="${ANDROID_SDK_ROOT:-${ANDROID_HOME:-}}"
CMDLINE_TOOLS_VERSION="${CMDLINE_TOOLS_VERSION:-13114758}"

if [[ ! -f "$GRADLE_PROPERTIES_FILE" ]]; then
  echo "❌ Missing file: $GRADLE_PROPERTIES_FILE"
  exit 1
fi

resolve_sdk_root() {
  local common_sdk_paths=(
    "${HOME}/Android/Sdk"
    "/usr/local/lib/android/sdk"
    "/opt/android-sdk"
    "/opt/android-sdk-linux"
  )

  if [[ -n "${ANDROID_HOME:-}" ]]; then
    echo "$ANDROID_HOME"
    return 0
  fi
  if [[ -n "${ANDROID_SDK_ROOT:-}" ]]; then
    echo "$ANDROID_SDK_ROOT"
    return 0
  fi
  for candidate in "${common_sdk_paths[@]}"; do
    if [[ -d "$candidate" ]]; then
      echo "$candidate"
      return 0
    fi
  done
  return 1
}

ensure_cmdline_tools() {
  local sdk_root="$1"
  local tools_bin="${sdk_root}/cmdline-tools/latest/bin"

  if [[ -x "${tools_bin}/sdkmanager" ]]; then
    export PATH="${tools_bin}:$PATH"
    return 0
  fi

  echo "ℹ️ sdkmanager not found. Bootstrapping Android command-line tools in ${sdk_root}"
  mkdir -p "${sdk_root}/cmdline-tools"

  local os_id=""
  case "$(uname -s)" in
    Linux) os_id="linux" ;;
    Darwin) os_id="mac" ;;
    *)
      echo "❌ Unsupported host OS for automatic cmdline-tools bootstrap: $(uname -s)"
      return 1
      ;;
  esac

  local archive="commandlinetools-${os_id}-${CMDLINE_TOOLS_VERSION}_latest.zip"
  local url="https://dl.google.com/android/repository/${archive}"
  local tmp_zip
  tmp_zip="$(mktemp)"

  if command -v curl >/dev/null 2>&1; then
    curl -fsSL "${url}" -o "${tmp_zip}"
  elif command -v wget >/dev/null 2>&1; then
    wget -qO "${tmp_zip}" "${url}"
  else
    echo "❌ Neither curl nor wget found to download Android cmdline-tools."
    rm -f "${tmp_zip}"
    return 1
  fi

  rm -rf "${sdk_root}/cmdline-tools/latest"
  mkdir -p "${sdk_root}/cmdline-tools/latest"
  unzip -q -o "${tmp_zip}" -d "${sdk_root}/cmdline-tools/latest"
  rm -f "${tmp_zip}"

  if [[ -d "${sdk_root}/cmdline-tools/latest/cmdline-tools" ]]; then
    local nested_dir="${sdk_root}/cmdline-tools/latest/cmdline-tools"
    cp -a "${nested_dir}/." "${sdk_root}/cmdline-tools/latest/"
    rm -rf "${nested_dir}"
  fi

  if [[ ! -x "${tools_bin}/sdkmanager" ]]; then
    echo "❌ sdkmanager still unavailable after bootstrap."
    return 1
  fi

  export PATH="${tools_bin}:$PATH"
}

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

ANDROID_SDK_PATH="$(resolve_sdk_root || true)"
if [[ -z "$ANDROID_SDK_PATH" && -n "${SDK_ROOT_OVERRIDE}" ]]; then
  ANDROID_SDK_PATH="${SDK_ROOT_OVERRIDE}"
fi
if [[ -z "$ANDROID_SDK_PATH" ]]; then
  ANDROID_SDK_PATH="${HOME}/Android/Sdk"
fi

if ! ensure_cmdline_tools "${ANDROID_SDK_PATH}"; then
  echo "❌ Failed to bootstrap Android command-line tools."
  exit 1
fi

if [[ -n "$ANDROID_SDK_PATH" ]]; then
  if [[ -f "$LOCAL_PROPERTIES_FILE" && "$(grep -E '^sdk.dir=' "$LOCAL_PROPERTIES_FILE" || true)" != "" ]]; then
    echo "ℹ️ Preserving existing sdk.dir in $LOCAL_PROPERTIES_FILE"
  else
    mkdir -p "$(dirname "$LOCAL_PROPERTIES_FILE")"
    printf 'sdk.dir=%s\n' "${ANDROID_SDK_PATH//\\/\\\\}" > "$LOCAL_PROPERTIES_FILE"
    echo "✅ Wrote sdk.dir to $LOCAL_PROPERTIES_FILE"
  fi
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
