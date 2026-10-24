#!/bin/sh
# Android ABI shared-link gate. Tools and JNI are HOSTED_BOUNDARY, not PURE_CORE.
set -eu
self=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
ndk=${ANDROID_NDK_HOME:-}
if [ -z "$ndk" ] && [ -n "${ANDROID_SDK_ROOT:-}" ]; then
  ndk="$ANDROID_SDK_ROOT/ndk/26.3.11579264"
fi
if [ -z "$ndk" ] && [ -n "${ANDROID_HOME:-}" ]; then
  ndk="$ANDROID_HOME/ndk/26.3.11579264"
fi
tool="$ndk/toolchains/llvm/prebuilt/linux-x86_64/bin"
test -x "$tool/armv7a-linux-androideabi24-clang" ||
  { echo "BLOCKED: pinned Android NDK r26.3 not installed" >&2; exit 5; }
test -x "$tool/aarch64-linux-android24-clang" ||
  { echo "BLOCKED: NDK lacks AArch64 Clang" >&2; exit 5; }
out=$(mktemp -d)
trap 'rm -rf "$out"' EXIT HUP INT TERM
common='-std=c11 -Os -fPIC -Wall -Wextra -Werror -ffreestanding -fno-builtin -fno-stack-protector -fno-unwind-tables -fno-asynchronous-unwind-tables'
for spec in 'armv7a-linux-androideabi24 ARM' 'aarch64-linux-android24 AArch64'; do
  set -- $spec
  compiler="$tool/$1-clang"
  "$compiler" $common -c "$self/vendor_pinned/raf_mw.c" -o "$out/$1.core.o"
  "$tool/llvm-nm" -u "$out/$1.core.o" > "$out/undefined"
  test ! -s "$out/undefined"
  "$compiler" $common -c "$self/raf_mw_jni.c" -o "$out/$1.bridge.o"
  "$compiler" -shared -Wl,--no-undefined -Wl,-z,max-page-size=16384 \
    "$out/$1.core.o" "$out/$1.bridge.o" \
    -o "$out/libtermux_rafaelia_middleware_s15.$1.so"
  "$tool/llvm-readelf" -h "$out/libtermux_rafaelia_middleware_s15.$1.so" |
    grep -q "Machine:.*$2"
  "$tool/llvm-nm" -D "$out/libtermux_rafaelia_middleware_s15.$1.so" |
    grep -q 'Java_com_termux_app_rafaelia_RafaeliaMiddlewareBridge_nativePlan'
  printf 'PASS ANDROID HOSTED ABI link: %s (NOT APK, NOT on-device runtime)\n' "$1"
done
