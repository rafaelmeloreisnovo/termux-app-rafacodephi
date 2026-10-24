#!/bin/sh
# HOSTED_TOOLING only. Runs real C fixtures and compiles JNI/Java, not Android APK.
set -eu
self=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
cc=${CC:-clang}
for cmd in "$cc" nm readelf git javac readlink; do
  command -v "$cmd" >/dev/null || { echo "BLOCKED: missing $cmd" >&2; exit 3; }
done
src="$self/vendor_pinned"
test "$(git hash-object "$src/raf_mw.c")" = 'e46b1e24191f901edf91d6ce4c5003fa5d74438f'
test "$(git hash-object "$src/raf_mw.h")" = 'ad97c24a2c9c3ea291e26ce3d74322f89d84f9d5'
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT HUP INT TERM
common='-std=c11 -Os -ffreestanding -fno-builtin -fno-stack-protector -fno-common -fno-unwind-tables -fno-asynchronous-unwind-tables -Wall -Wextra -Werror'
"$cc" -std=c11 -Wall -Wextra -Werror -fsanitize=address,undefined \
  "$src/raf_mw.c" "$self/tests/test_route.c" -o "$work/core_test"
"$work/core_test"
for target in x86_64-unknown-linux-gnu armv7a-linux-gnueabi aarch64-linux-gnu; do
  "$cc" -target "$target" $common -c "$src/raf_mw.c" -o "$work/$target.o"
  nm -u "$work/$target.o" > "$work/undefined"
  test ! -s "$work/undefined"
  readelf -h "$work/$target.o" | grep 'Machine:'
  printf 'PASS L0 OBJECT %s: no external symbol (runtime NOT_RUN)\n' "$target"
done
# Compiler syntax check of real JNI API using the host JDK headers.
# Android NDK shared link is deliberately a separate gate.
jdk=$(CDPATH= cd -- "$(dirname "$(readlink -f "$(command -v javac)")")/.." && pwd)
test -f "$jdk/include/jni.h" || { echo 'BLOCKED: host JDK JNI headers absent' >&2; exit 4; }
"$cc" -std=c11 -Wall -Wextra -Werror -I"$jdk/include" -I"$jdk/include/linux" \
  -c "$self/raf_mw_jni.c" -o "$work/bridge_host.o"
java_file="$self/../../../java/com/termux/app/rafaelia/RafaeliaMiddlewareBridge.java"
test -f "$java_file"
javac -d "$work" "$java_file"
# Build contract: feature disabled by default; no modification to old bootstrap path.
mk="$self/../../Android.mk"
grep -Fq 'RAFAELIA_S15_MIDDLEWARE ?= 0' "$mk"
grep -Fq 'ifeq ($(RAFAELIA_S15_MIDDLEWARE),1)' "$mk"
printf 'PASS HOST ONLY: source SHA pin, 14+4 fixtures, C JNI syntax, Java syntax, default-off build\n'
printf 'TOKEN_VAZIO: NDK link, APK artifact, ARM32/ARM64 device, CPU/GPU dispatch, live failover\n'
