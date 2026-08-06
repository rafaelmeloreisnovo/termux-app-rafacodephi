#!/bin/sh
# environment_identity_failsafe.sh
# R0 — Validação de identidade do ambiente RAFCODEΦ
# Garante que operações críticas rodem APENAS dentro de com.termux.rafacodephi
# Exit 42 se identidade errada; 0 se OK

set -e

EXPECTED_PREFIX="/data/data/com.termux.rafacodephi/files/usr"
EXPECTED_PACKAGE="com.termux.rafacodephi"

actual_prefix="${PREFIX:-/UNSET}"
actual_home="${HOME:-/UNSET}"
actual_uid="$(id -u 2>/dev/null || echo UNSET)"
actual_arch="$(uname -m 2>/dev/null || echo UNSET)"

# Estratégia 1: Validar $PREFIX (mais rápido, sem system calls)
if [ "$actual_prefix" != "$EXPECTED_PREFIX" ]; then
  cat >&2 << EOF
ERROR: wrong_environment
  expected_prefix=$EXPECTED_PREFIX
  actual_prefix=$actual_prefix
  uid=$actual_uid
  arch=$actual_arch
  timestamp=$(date -u +%Y-%m-%dT%H:%M:%SZ 2>/dev/null || echo UNSET)
EOF
  exit 42
fi

# Estratégia 2: Se $PREFIX correto mas ambiente parece fora (extra safety)
if [ ! -d "$actual_prefix" ]; then
  cat >&2 << EOF
ERROR: prefix_dir_missing
  expected=$EXPECTED_PREFIX
  actual=$actual_prefix
  status=not_a_directory
EOF
  exit 42
fi

# OK — Emitir receipt
{
  echo "identity_ok=true"
  echo "package=$EXPECTED_PACKAGE"
  echo "prefix=$actual_prefix"
  echo "home=$actual_home"
  echo "uid=$actual_uid"
  echo "arch=$actual_arch"
  echo "timestamp=$(date -u +%Y-%m-%dT%H:%M:%SZ 2>/dev/null || echo UNSET)"
} >&1

exit 0
