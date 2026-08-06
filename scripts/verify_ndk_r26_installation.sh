#!/bin/sh
# verify_ndk_r26_installation.sh
# R3.2 — Verify Android NDK installation matches gradle.properties (read-only auditor)
#
# Does NOT install anything (see scripts/setup_android_toolchain.sh for that).
# Runs on the BUILD HOST (Linux x86_64 dev/CI machine), not on-device — R0's
# environment_identity_failsafe.sh does not gate this script.
#
# Emits a structured receipt to stdout and exits:
#   0  = NDK found, version matches, required cross-toolchains present
#   1  = NDK missing, version mismatch, or required target compiler missing

set -e

GRADLE_PROPERTIES_FILE="${1:-gradle.properties}"

if [ ! -f "$GRADLE_PROPERTIES_FILE" ]; then
  echo "ERROR: gradle_properties_not_found file=$GRADLE_PROPERTIES_FILE" >&2
  exit 1
fi

read_prop() {
  key="$1"
  grep -E "^${key}=" "$GRADLE_PROPERTIES_FILE" 2>/dev/null | head -n1 | cut -d= -f2- | tr -d '[:space:]'
}

EXPECTED_NDK_VERSION="$(read_prop ndkVersion)"
COMPILE_SDK_VERSION="$(read_prop compileSdkVersion)"
TARGET_SDK_VERSION="$(read_prop targetSdkVersion)"
MIN_SDK_VERSION="$(read_prop minSdkVersion)"

if [ -z "$EXPECTED_NDK_VERSION" ]; then
  echo "ERROR: ndkVersion_not_set_in_gradle_properties" >&2
  exit 1
fi

# Resolve candidate SDK roots (mirrors scripts/setup_android_toolchain.sh)
SDK_ROOT="${ANDROID_SDK_ROOT:-${ANDROID_HOME:-}}"
if [ -z "$SDK_ROOT" ]; then
  for candidate in "${HOME}/Android/Sdk" "/usr/local/lib/android/sdk" "/opt/android-sdk" "/opt/android-sdk-linux"; do
    [ -d "$candidate" ] && { SDK_ROOT="$candidate"; break; }
  done
fi

# Resolve NDK path: explicit override wins, else derive from SDK_ROOT/ndk/<version>
NDK_PATH=""
NDK_SOURCE=""
if [ -n "$ANDROID_NDK" ] && [ -d "$ANDROID_NDK" ]; then
  NDK_PATH="$ANDROID_NDK"
  NDK_SOURCE="ANDROID_NDK"
elif [ -n "$ANDROID_NDK_HOME" ] && [ -d "$ANDROID_NDK_HOME" ]; then
  NDK_PATH="$ANDROID_NDK_HOME"
  NDK_SOURCE="ANDROID_NDK_HOME"
elif [ -n "$ANDROID_NDK_ROOT" ] && [ -d "$ANDROID_NDK_ROOT" ]; then
  NDK_PATH="$ANDROID_NDK_ROOT"
  NDK_SOURCE="ANDROID_NDK_ROOT"
elif [ -n "$SDK_ROOT" ] && [ -d "${SDK_ROOT}/ndk/${EXPECTED_NDK_VERSION}" ]; then
  NDK_PATH="${SDK_ROOT}/ndk/${EXPECTED_NDK_VERSION}"
  NDK_SOURCE="SDK_ROOT/ndk/<version>"
fi

TIMESTAMP=$(date -u +%Y-%m-%dT%H:%M:%SZ 2>/dev/null || echo UNSET)

if [ -z "$NDK_PATH" ]; then
  cat << EOF
=== R3.2 NDK Verification Receipt ===
timestamp=$TIMESTAMP
status=NDK_NOT_FOUND
expected_ndk_version=$EXPECTED_NDK_VERSION
sdk_root=${SDK_ROOT:-UNSET}
searched=\$ANDROID_NDK,\$ANDROID_NDK_HOME,\$ANDROID_NDK_ROOT,\${SDK_ROOT}/ndk/${EXPECTED_NDK_VERSION}
remediation=Run scripts/setup_android_toolchain.sh to install ndk;${EXPECTED_NDK_VERSION} via sdkmanager
r3_gate=R3.2_BLOCKED
EOF
  exit 1
fi

# Confirm actual installed version via source.properties (authoritative over path name)
ACTUAL_NDK_VERSION="UNKNOWN"
if [ -f "${NDK_PATH}/source.properties" ]; then
  ACTUAL_NDK_VERSION=$(grep '^Pkg.Revision' "${NDK_PATH}/source.properties" 2>/dev/null | cut -d= -f2 | tr -d '[:space:]')
fi

VERSION_MATCH="false"
[ "$ACTUAL_NDK_VERSION" = "$EXPECTED_NDK_VERSION" ] && VERSION_MATCH="true"

# Locate the toolchain prebuilt dir (host triplet varies; try common ones)
TOOLCHAIN_BIN=""
for host_triplet in linux-x86_64 darwin-x86_64 windows-x86_64; do
  candidate="${NDK_PATH}/toolchains/llvm/prebuilt/${host_triplet}/bin"
  [ -d "$candidate" ] && { TOOLCHAIN_BIN="$candidate"; break; }
done

# Required cross-compilers per RAFCODEΦ ABI matrix (armeabi-v7a, arm64-v8a)
# API level pinned to targetSdkVersion (Termux-style execution floor), per gradle.properties comment
API_LEVEL="${TARGET_SDK_VERSION:-28}"
ARM64_CC="${TOOLCHAIN_BIN}/aarch64-linux-android${API_LEVEL}-clang"
ARM32_CC="${TOOLCHAIN_BIN}/armv7a-linux-androideabi${API_LEVEL}-clang"

ARM64_CC_OK="false"
ARM32_CC_OK="false"
[ -n "$TOOLCHAIN_BIN" ] && [ -x "$ARM64_CC" ] && ARM64_CC_OK="true"
[ -n "$TOOLCHAIN_BIN" ] && [ -x "$ARM32_CC" ] && ARM32_CC_OK="true"

CLANG_VERSION="UNKNOWN"
if [ "$ARM64_CC_OK" = "true" ]; then
  CLANG_VERSION=$("$ARM64_CC" --version 2>/dev/null | head -1)
fi

# Overall pass/fail
OVERALL="PASS"
[ "$VERSION_MATCH" = "true" ] || OVERALL="FAIL"
[ "$ARM64_CC_OK" = "true" ] || OVERALL="FAIL"
[ "$ARM32_CC_OK" = "true" ] || OVERALL="FAIL"

cat << EOF
=== R3.2 NDK Verification Receipt ===
timestamp=$TIMESTAMP
status=NDK_FOUND
ndk_path=$NDK_PATH
ndk_source=$NDK_SOURCE
expected_ndk_version=$EXPECTED_NDK_VERSION
actual_ndk_version=$ACTUAL_NDK_VERSION
version_match=$VERSION_MATCH
compile_sdk_version=$COMPILE_SDK_VERSION
target_sdk_version=$TARGET_SDK_VERSION
min_sdk_version=$MIN_SDK_VERSION
api_level_for_toolchain=$API_LEVEL
toolchain_bin=${TOOLCHAIN_BIN:-NOT_FOUND}
arm64_compiler=$ARM64_CC
arm64_compiler_present=$ARM64_CC_OK
arm32_compiler=$ARM32_CC
arm32_compiler_present=$ARM32_CC_OK
clang_version=$CLANG_VERSION
overall=$OVERALL
r3_gate=$( [ "$OVERALL" = "PASS" ] && echo R3.2_COMPLETE || echo R3.2_BLOCKED )
EOF

[ "$OVERALL" = "PASS" ] && exit 0 || exit 1
