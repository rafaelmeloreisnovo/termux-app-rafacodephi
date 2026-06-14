#!/data/data/com.termux/files/usr/bin/sh
# BUILD.sh — freestanding APK compiler build for Termux (clang)
# No libc, no crt, no heap. arm64-v8a + armeabi-v7a dual target.
set -e

CC=clang
CFLAGS="-std=c11 -O2 -ffreestanding -nostdlib -nostartfiles
  -fno-stack-protector -fno-unwind-tables -fno-asynchronous-unwind-tables
  -fno-builtin -fomit-frame-pointer -Wall -Wextra"

LDFLAGS="-nostdlib -nostartfiles -static -Wl,--build-id=none,-s"

DIR="$(cd "$(dirname "$0")" && pwd)"

build_arch() {
    ARCH="$1"   # aarch64 or armv7a
    TRIPLE="$2" # aarch64-linux-android or armv7a-linux-androideabi
    OUT="$3"    # apkc64 or apkc32
    echo "[build] $ARCH -> $OUT"
    $CC $CFLAGS --target="$TRIPLE21" \
        -I"$DIR" \
        -e _start \
        -Wl,-Ttext-segment=0x10000 \
        $LDFLAGS \
        -o "$DIR/$OUT" \
        "$DIR/apkc.c"
    echo "[ok] $OUT"
}

build_arch aarch64   aarch64-linux-android   apkc64
build_arch armv7a    armv7a-linux-androideabi apkc32

echo
echo "Usage examples:"
echo "  ./apkc64 examples/hello.s -o hello.apk -p com.hello -l Hello -n hello"
echo "  ./apkc32 examples/hello.s -o hello32.apk -p com.hello -l Hello -n hello"
echo "  ./apkc64 examples/hello.s -o hello_both.apk -both"
