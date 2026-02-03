#!/usr/bin/env bash
set -euo pipefail

cc="${CC:-clang}"

echo "[1] hosted demo" 
$cc -O2 -Wall -Wextra demo_hosted.c rmr_compat.c -o demo_hosted
./demo_hosted

echo "[2] freestanding object sanity" 
$cc -O2 -Wall -Wextra -ffreestanding -fno-builtin -nostdlib -c rmr_compat.c -o rmr_compat.o
nm -u rmr_compat.o || true
nm -g --defined-only rmr_compat.o | sort | head -n 50

echo "OK"
