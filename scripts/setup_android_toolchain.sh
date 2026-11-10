#!/usr/bin/env bash
set -euo pipefail

GRADLE_PROPERTIES_FILE="${1:-gradle.properties}"
LOCAL_PROPERTIES_FILE="${2:-local.properties}"
SDK_ROOT_OVERRIDE="${ANDROID_SDK_ROOT:-${ANDROID_HOME:-}}"
CMDLINE_TOOLS_VERSION="${CMDLINE_TOOLS_VERSION:-13114758}"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"

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
CANONICAL_NDK_VERSION="$(read_prop ndkVersion)"
EFFECTIVE_NDK_VERSION="${JITPACK_NDK_VERSION:-$CANONICAL_NDK_VERSION}"
BUILD_TOOLS_VERSION="$(read_prop buildToolsVersion)"

if [[ -z "$COMPILE_SDK_VERSION" || -z "$TARGET_SDK_VERSION" || -z "$CANONICAL_NDK_VERSION" ]]; then
  echo "❌ Missing required Android toolchain keys in $GRADLE_PROPERTIES_FILE"
  echo "Required: compileSdkVersion, targetSdkVersion, ndkVersion"
  exit 1
fi

if [[ ! "$EFFECTIVE_NDK_VERSION" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
  echo "❌ Invalid effective NDK version: $EFFECTIVE_NDK_VERSION" >&2
  exit 1
fi

if [[ -z "$BUILD_TOOLS_VERSION" ]]; then
  BUILD_TOOLS_VERSION="${COMPILE_SDK_VERSION}.0.0"
fi

echo "📦 Android toolchain source of truth: $GRADLE_PROPERTIES_FILE"
echo "compileSdkVersion=$COMPILE_SDK_VERSION"
echo "targetSdkVersion=$TARGET_SDK_VERSION"
echo "canonicalNdkVersion=$CANONICAL_NDK_VERSION"
echo "effectiveNdkVersion=$EFFECTIVE_NDK_VERSION"
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

SDK_PACKAGES=(
  "platform-tools"
  "platforms;android-${COMPILE_SDK_VERSION}"
  "build-tools;${BUILD_TOOLS_VERSION}"
  "ndk;${EFFECTIVE_NDK_VERSION}"
  "cmake;3.22.1"
)

sdk_install_ok=false
for attempt in 1 2; do
  if sdkmanager "${SDK_PACKAGES[@]}"; then
    sdk_install_ok=true
    break
  fi
  echo "⚠️ Android SDK installation attempt ${attempt}/2 failed" >&2
done
if [[ "$sdk_install_ok" != true ]]; then
  echo "❌ Android SDK/NDK package installation failed after two attempts" >&2
  exit 1
fi

ANDROID_NDK_PATH="${ANDROID_SDK_PATH}/ndk/${EFFECTIVE_NDK_VERSION}"
ANDROID_BUILD_TOOLS_PATH="${ANDROID_SDK_PATH}/build-tools/${BUILD_TOOLS_VERSION}"
ANDROID_PLATFORM_TOOLS_PATH="${ANDROID_SDK_PATH}/platform-tools"
ANDROID_CMDLINE_TOOLS_PATH="${ANDROID_SDK_PATH}/cmdline-tools/latest/bin"
ANDROID_CMAKE_PATH="${ANDROID_SDK_PATH}/cmake/3.22.1"
ANDROID_APKSIGNER_PATH="${ANDROID_BUILD_TOOLS_PATH}/apksigner"

NDK_PREBUILT_BIN=""
for candidate in "${ANDROID_NDK_PATH}"/toolchains/llvm/prebuilt/*/bin; do
  if [[ -d "$candidate" ]]; then
    NDK_PREBUILT_BIN="$candidate"
    break
  fi
done

for executable in \
  "${ANDROID_PLATFORM_TOOLS_PATH}/adb" \
  "${ANDROID_PLATFORM_TOOLS_PATH}/fastboot" \
  "${ANDROID_NDK_PATH}/ndk-build" \
  "${ANDROID_CMAKE_PATH}/bin/ninja" \
  "$ANDROID_APKSIGNER_PATH"; do
  if [[ ! -x "$executable" ]]; then
    echo "❌ Required Android tool is not executable: $executable" >&2
    exit 1
  fi
done
if [[ -z "$NDK_PREBUILT_BIN" ]]; then
  echo "❌ NDK LLVM prebuilt bin directory was not found under $ANDROID_NDK_PATH" >&2
  exit 1
fi

export ANDROID_HOME="$ANDROID_SDK_PATH"
export ANDROID_SDK_ROOT="$ANDROID_SDK_PATH"
export ANDROID_NDK_HOME="$ANDROID_NDK_PATH"
export ANDROID_NDK_ROOT="$ANDROID_NDK_PATH"
export ANDROID_APKSIGNER="$ANDROID_APKSIGNER_PATH"
export APKSIGNER="$ANDROID_APKSIGNER_PATH"
export PATH="${ANDROID_CMDLINE_TOOLS_PATH}:${ANDROID_PLATFORM_TOOLS_PATH}:${ANDROID_BUILD_TOOLS_PATH}:${ANDROID_CMAKE_PATH}/bin:${ANDROID_NDK_PATH}:${NDK_PREBUILT_BIN}:$PATH"

if [[ -n "${GITHUB_ENV:-}" ]]; then
  {
    printf 'ANDROID_HOME=%s\n' "$ANDROID_HOME"
    printf 'ANDROID_SDK_ROOT=%s\n' "$ANDROID_SDK_ROOT"
    printf 'ANDROID_NDK_HOME=%s\n' "$ANDROID_NDK_HOME"
    printf 'ANDROID_NDK_ROOT=%s\n' "$ANDROID_NDK_ROOT"
    printf 'ANDROID_APKSIGNER=%s\n' "$ANDROID_APKSIGNER"
    printf 'APKSIGNER=%s\n' "$APKSIGNER"
    printf 'RAFCODEPHI_EFFECTIVE_NDK_VERSION=%s\n' "$EFFECTIVE_NDK_VERSION"
  } >> "$GITHUB_ENV"
fi
if [[ -n "${GITHUB_PATH:-}" ]]; then
  {
    printf '%s\n' "$ANDROID_CMDLINE_TOOLS_PATH"
    printf '%s\n' "$ANDROID_PLATFORM_TOOLS_PATH"
    printf '%s\n' "$ANDROID_BUILD_TOOLS_PATH"
    printf '%s\n' "${ANDROID_CMAKE_PATH}/bin"
    printf '%s\n' "$ANDROID_NDK_PATH"
    printf '%s\n' "$NDK_PREBUILT_BIN"
  } >> "$GITHUB_PATH"
fi

adb version | sed -n '1p'
fastboot --version | sed -n '1p'
ndk-build --version | sed -n '1p'
ninja --version

ensure_ci_blake3() {
  command -v b3sum >/dev/null 2>&1 && return 0

  if ! command -v sudo >/dev/null 2>&1 || ! command -v apt-get >/dev/null 2>&1; then
    echo "❌ b3sum is required for strict GitHub Actions bootstrap hashing, but no supported installer is available."
    return 1
  fi

  echo "📦 Installing b3sum for strict GitHub Actions bootstrap hashing"
  sudo apt-get update
  sudo apt-get install --yes --no-install-recommends b3sum
  command -v b3sum >/dev/null 2>&1 || {
    echo "❌ b3sum remained unavailable after package installation."
    return 1
  }
  b3sum --version
}

# GitHub Actions that use this legacy/general toolchain helper and then invoke
# Gradle must materialize the same rewritten bootstraps as the canonical
# RAFAELIA setup action. This closes the .incbin divergence without changing
# local developer semantics. Set TERMUX_SKIP_BOOTSTRAP_PREPARE=1 only for jobs
# that provably do not build the app/native bootstrap target.
if [[ "${GITHUB_ACTIONS:-false}" == "true" && "${TERMUX_SKIP_BOOTSTRAP_PREPARE:-0}" != "1" ]]; then
  ensure_ci_blake3
  BOOTSTRAP_PREPARE_SCRIPT="${ROOT_DIR}/scripts/prepare_bootstrap_env.sh"
  if [[ ! -x "$BOOTSTRAP_PREPARE_SCRIPT" ]]; then
    echo "❌ Missing executable bootstrap preparation script: $BOOTSTRAP_PREPARE_SCRIPT"
    exit 1
  fi
  echo "🧬 Preparing canonical rewritten bootstraps for GitHub Actions build"
  "$BOOTSTRAP_PREPARE_SCRIPT" --github-env --skip-android-preflight
fi
