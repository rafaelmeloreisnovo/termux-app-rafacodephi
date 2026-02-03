#!/data/data/com.termux/files/usr/bin/bash
set -euo pipefail

echo "[*] Installing tools..."
pkg update -y
pkg install -y clang make

echo "[*] Build baseline (no BLAKE3)..."
clang -O3 -DNDEBUG -std=c11 -Wall -Wextra -pedantic bench_rafaelia.c -o bench_rafaelia

echo
echo "[*] Running baseline..."
./bench_rafaelia --size 16777216 --rounds 50

echo
echo "[*] If you have FULL BLAKE3 sources (including required internal headers), build with HAVE_BLAKE3 like this:"
echo "    clang -O3 -DNDEBUG -std=c11 -Wall -Wextra -pedantic -DHAVE_BLAKE3 \\"
echo "      bench_rafaelia.c blake3.c blake3_dispatch.c blake3_portable.c blake3_neon.c \\"
echo "      -o bench_rafaelia_blake3"
echo "    ./bench_rafaelia_blake3 --size 16777216 --rounds 30"

echo
echo "[*] Optional IO test (Termux HOME file):"
echo "    ./bench_rafaelia --size 8388608 --rounds 20 --io ./bench.tmp"
