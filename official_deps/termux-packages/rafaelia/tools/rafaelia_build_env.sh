#!/data/data/com.termux/files/usr/bin/bash
# RAFAELIA Build Env — performance knobs for termux-packages builds
# Safe defaults: only exports vars; does not modify upstream scripts.

set -euo pipefail

# Use as:
#   source rafaelia/tools/rafaelia_build_env.sh
#   ./scripts/run-docker.sh ./build-package.sh -a aarch64 <pkg>

# Parallelism
if command -v nproc >/dev/null 2>&1; then
  export RAF_JOBS="${RAF_JOBS:-$(nproc)}"
else
  export RAF_JOBS="${RAF_JOBS:-4}"
fi
export MAKEFLAGS="${MAKEFLAGS:--j${RAF_JOBS}}"

# Prefer clang if available
export CC="${CC:-clang}"
export CXX="${CXX:-clang++}"

# ccache (optional)
export USE_CCACHE="${USE_CCACHE:-true}"
export CCACHE_DIR="${CCACHE_DIR:-$HOME/.cache/ccache}"
export CCACHE_COMPRESS="${CCACHE_COMPRESS:-true}"
export CCACHE_MAXSIZE="${CCACHE_MAXSIZE:-10G}"

# LTO/strip (opt-in)
export RAF_LTO="${RAF_LTO:-0}"   # 1 to enable where supported
export RAF_STRIP="${RAF_STRIP:-1}" # 1 to strip binaries in packaging step

# Compression
export RAF_COMPRESS="${RAF_COMPRESS:-zstd}" # zstd|xz|gz
export RAF_ZSTD_LEVEL="${RAF_ZSTD_LEVEL:-8}" # good speed/ratio balance

# Reproducibility toggles
export SOURCE_DATE_EPOCH="${SOURCE_DATE_EPOCH:-0}"

echo "[RAFAELIA] jobs=$RAF_JOBS cc=$CC ccache=$USE_CCACHE compress=$RAF_COMPRESS"
