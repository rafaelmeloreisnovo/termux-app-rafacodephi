#!/usr/bin/env sh
set -eu

ROOT=${1:-.}
SRC="$ROOT/app/src/main/cpp/freestanding"
OUT=$(mktemp -d)
trap 'rm -rf "$OUT"' EXIT HUP INT TERM

for token in '#include' 'malloc(' 'calloc(' 'realloc(' 'free(' 'JNIEnv' 'JNIEXPORT' '__android_log' 'printf(' 'fopen(' 'dlopen('; do
    if grep -R -n -F "$token" "$SRC"; then
        echo "forbidden token: $token" >&2
        exit 1
    fi
done

clang --target=aarch64-linux-android21 -O3 -ffreestanding -fno-builtin \
  -fno-stack-protector -fno-unwind-tables -fno-asynchronous-unwind-tables \
  -fvisibility=hidden -c "$SRC/raf_pa_core.c" -o "$OUT/core64.o"
clang --target=aarch64-linux-android21 -c "$SRC/raf_pa_entry_arm64.S" -o "$OUT/entry64.o"
ld.lld -shared -nostdlib -e _start --gc-sections --build-id=none \
  -z max-page-size=16384 -z common-page-size=16384 \
  "$OUT/entry64.o" "$OUT/core64.o" -o "$OUT/libraf_pa_core_arm64.so"

clang --target=armv7a-linux-androideabi21 -O3 -ffreestanding -fno-builtin \
  -fno-stack-protector -fno-unwind-tables -fno-asynchronous-unwind-tables \
  -fvisibility=hidden -mfloat-abi=softfp -mfpu=neon-vfpv4 \
  -c "$SRC/raf_pa_core.c" -o "$OUT/core32.o"
clang --target=armv7a-linux-androideabi21 -c "$SRC/raf_pa_entry_arm32.S" -o "$OUT/entry32.o"
ld.lld -shared -nostdlib -e _start --gc-sections --build-id=none \
  -z max-page-size=16384 -z common-page-size=16384 \
  "$OUT/entry32.o" "$OUT/core32.o" -o "$OUT/libraf_pa_core_arm32.so"

for elf in "$OUT/libraf_pa_core_arm64.so" "$OUT/libraf_pa_core_arm32.so"; do
    readelf -h "$elf" | grep -q 'Entry point address: *0x[1-9a-fA-F]'
    if readelf -d "$elf" | grep -q '(NEEDED)'; then
        echo "DT_NEEDED found in $elf" >&2
        exit 1
    fi
    if readelf -Ws "$elf" | awk '$7 == "UND" && $8 != "" { bad=1 } END { exit bad }'; then
        :
    else
        echo "undefined external symbol found in $elf" >&2
        exit 1
    fi
done

printf '%s\n' 'PA_FREESTANDING_ELF_PASS'
printf '%s\n' 'ARM32=ELF32_ARM_EABI5_ENTRY_NO_NEEDED_NO_UNDEF'
printf '%s\n' 'ARM64=ELF64_AARCH64_ENTRY_NO_NEEDED_NO_UNDEF'
