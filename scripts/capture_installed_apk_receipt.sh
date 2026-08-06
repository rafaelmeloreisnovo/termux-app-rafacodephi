#!/bin/sh
# capture_installed_apk_receipt.sh
# R1 — Captura de receipt do APK RAFCODEΦ instalado
# Deve rodar DENTRO do ambiente RAFCODEΦ (validado por R0)
# Gera arquivo append-only em $HOME/.rafcodephi/receipts/

set -e

# R0: Validação de identidade
if ! . "$(dirname "$0")/environment_identity_failsafe.sh" > /dev/null 2>&1; then
  echo "FATAL: Not running in RAFCODEΦ environment" >&2
  exit 42
fi

PACKAGE="com.termux.rafacodephi"
RECEIPT_DIR="${HOME}/.rafcodephi/receipts"
mkdir -p "$RECEIPT_DIR"

# Tentativa 1: pm path (mais confiável em Android)
APK_PATH=""
if command -v pm > /dev/null 2>&1; then
  APK_PATH=$(pm path "$PACKAGE" 2>/dev/null | sed 's/package://')
fi

# Tentativa 2: dumpsys (fallback)
if [ -z "$APK_PATH" ] && command -v dumpsys > /dev/null 2>&1; then
  APK_PATH=$(dumpsys package "$PACKAGE" 2>/dev/null | grep "codePath=" | head -1 | cut -d= -f2 | cut -d' ' -f1)
fi

# Se ainda não encontrou, FAIL
if [ -z "$APK_PATH" ] || [ ! -f "$APK_PATH" ]; then
  cat >&2 << EOF
ERROR: APK_NOT_FOUND
  package=$PACKAGE
  pm_path_status=${APK_PATH:-not_found}
  accessible=$([ -f "$APK_PATH" ] && echo true || echo false)
EOF
  exit 1
fi

# SHA-256 do APK
APK_SHA256=$(sha256sum "$APK_PATH" 2>/dev/null | cut -d' ' -f1)

# Metadados via dumpsys (versão, ABIs, timestamps)
METADATA=""
if command -v dumpsys > /dev/null 2>&1; then
  METADATA=$(dumpsys package "$PACKAGE" 2>/dev/null | grep -E 'versionName=|versionCode=|firstInstallTime=|lastUpdateTime=|primaryCpuAbi=|secondaryCpuAbi=' || true)
fi

# Timestamp ISO 8601 UTC
TIMESTAMP=$(date -u +%Y-%m-%dT%H:%M:%SZ 2>/dev/null || echo UNSET)
RECEIPT_BASE=$(date -u +%Y%m%dT%H%M%SZ 2>/dev/null || echo unset)

# Gravar receipt append-only
RECEIPT_FILE="${RECEIPT_DIR}/apk_installed_${RECEIPT_BASE}.receipt"

{
  echo "timestamp=$TIMESTAMP"
  echo "package=$PACKAGE"
  echo "apk_path=$APK_PATH"
  echo "apk_sha256=$APK_SHA256"
  echo "arch=$(uname -m 2>/dev/null || echo UNSET)"
  echo "uid=$(id -u 2>/dev/null || echo UNSET)"
  [ -n "$METADATA" ] && echo "$METADATA"
} | tee "$RECEIPT_FILE"

# Emitir status para auditoria
echo ""
echo "=== R1 Receipt Capture Complete ==="
echo "receipt_file=$RECEIPT_FILE"
echo "receipt_size=$(stat -c%s "$RECEIPT_FILE" 2>/dev/null || echo UNKNOWN)"
echo "receipt_hash=$(sha256sum "$RECEIPT_FILE" 2>/dev/null | cut -d' ' -f1)"

exit 0
