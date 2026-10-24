#!/bin/sh
# HOSTED_BUILD only. Verify the existing Android.mk can actually select S15.
# This does not assemble, sign, install or launch a Termux APK.
set -eu
self=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
ndk=${ANDROID_NDK_HOME:-}
if [ -z "$ndk" ] && [ -n "${ANDROID_SDK_ROOT:-}" ]; then
  ndk="$ANDROID_SDK_ROOT/ndk/26.3.11579264"
fi
if [ -z "$ndk" ] && [ -n "${ANDROID_HOME:-}" ]; then
  ndk="$ANDROID_HOME/ndk/26.3.11579264"
fi
test -x "$ndk/ndk-build" || { echo "BLOCKED: NDK ndk-build absent" >&2; exit 5; }
mk=$(CDPATH= cd -- "$self/../.." && pwd)/Android.mk
test -f "$mk"
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT HUP INT TERM
RAFAELIA_S15_MIDDLEWARE=1 "$ndk/ndk-build" \
  NDK_PROJECT_PATH="$work/project" \
  APP_BUILD_SCRIPT="$mk" \
  NDK_OUT="$work/obj" \
  NDK_LIBS_OUT="$work/libs" \
  APP_PLATFORM=android-24 APP_ABI="armeabi-v7a arm64-v8a" \
  -j2 termux_rafaelia_middleware_s15
tool="$ndk/toolchains/llvm/prebuilt/linux-x86_64/bin"
for pair in 'armeabi-v7a ARM' 'arm64-v8a AArch64'; do
  set -- $pair
  artifact="$work/libs/$1/libtermux_rafaelia_middleware_s15.so"
  test -s "$artifact"
  "$tool/llvm-readelf" -h "$artifact" | grep -q "Machine:.*$2"
  "$tool/llvm-nm" -D "$artifact" |
    grep -q 'Java_com_termux_app_rafaelia_RafaeliaMiddlewareBridge_nativePlan'
  printf 'PASS Android.mk opt-in native module ABI %s (no APK or device)\n' "$1"
done
