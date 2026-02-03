#!/usr/bin/env bash
# RAFAELIA :: environment doctor (Termux host + proot)
set -euo pipefail

say(){ printf '%s\n' "$*"; }
kv(){ printf '%-18s %s\n' "$1" "$2"; }

say '== RAFAELIA DOCTOR =='
kv 'DATE' "$(date -Iseconds 2>/dev/null || date)"
kv 'SHELL' "${SHELL:-?}"
kv 'PREFIX' "${PREFIX:-<none>}"
kv 'HOME' "${HOME:-<none>}"
kv 'PWD' "$(pwd)"

say ''
say '-- TERMUX HOST --'
uname -a || true

need=(pkg git clang llvm-ar llvm-nm ld.lld make cmake ninja pkg-config python jq file rg proot proot-distro)
for x in "${need[@]}"; do
  if command -v "$x" >/dev/null 2>&1; then
    kv "$x" "$(command -v "$x")"
  else
    kv "$x" 'MISSING'
  fi
done

say ''
if command -v pkg >/dev/null 2>&1; then
  kv 'pkg installed count' "$(pkg list-installed 2>/dev/null | wc -l | tr -d ' ')"
fi

say ''
say '-- PROOT DISTROS --'
if command -v proot-distro >/dev/null 2>&1; then
  proot-distro list | sed -n '1,30p' || true
else
  say 'proot-distro: MISSING'
fi

say ''
say '-- QUICK GUIDANCE --'
cat <<'EOT'
HOST (Termux): use for freestanding/core + scripts
  ./rafaelia_env/bin/rafx host <cmd>
PROOT (Linux): use for glibc/musl + /usr paths
  ./rafaelia_env/bin/rafx debian <cmd>
EOT
