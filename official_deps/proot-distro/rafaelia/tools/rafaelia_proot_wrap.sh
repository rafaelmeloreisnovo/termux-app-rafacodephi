#!/data/data/com.termux/files/usr/bin/bash
# RAFAELIA PROOT WRAP — performance/compat knobs for proot-distro
# Non-invasive: wraps proot-distro without modifying its core script.

set -euo pipefail

if ! command -v proot-distro >/dev/null 2>&1; then
  echo "proot-distro not found in PATH. Install it or run from Termux." >&2
  exit 1
fi

DISTRO="${1:-}"
shift || true

if [[ -z "$DISTRO" ]]; then
  echo "Usage: $0 <distro> [-- <command>]" >&2
  echo "Example: $0 debian -- bash" >&2
  exit 2
fi

# Put temp & cache on internal fast storage
export PROOT_TMP_DIR="${PROOT_TMP_DIR:-$PREFIX/tmp}"
export TMPDIR="${TMPDIR:-$PREFIX/tmp}"
mkdir -p "$PROOT_TMP_DIR"

# Favor fewer syscalls where possible
# link2symlink can reduce overhead on some Android filesystems
export PROOT_OPTIONS="${PROOT_OPTIONS:---link2symlink}"

# Optional: disable seccomp emu if it causes overhead / issues (opt-in)
# export PROOT_NO_SECCOMP=1

# Run distro login with extra options
exec proot-distro login "$DISTRO" --shared-tmp --termux-home -- $@
