#!/bin/sh
# link_receipt_to_commit.sh
# R2 — Vinculação de receipt a commit produtor para rastreabilidade determinística
# Entrada: arquivo de receipt de R1
# Saída: receipt vinculado com producer_commit + confidence + chain_hash

set -e

RECEIPT_IN="${1:?usage: link_receipt_to_commit.sh <apk_receipt_file>}"

# Validação: arquivo existe?
if [ ! -f "$RECEIPT_IN" ]; then
  cat >&2 << EOF
ERROR: receipt_file_not_found
  file=$RECEIPT_IN
EOF
  exit 1
fi

# Extrair SHA-256 do APK do receipt
APK_SHA256=$(grep "^apk_sha256=" "$RECEIPT_IN" 2>/dev/null | cut -d= -f2 | head -1)

if [ -z "$APK_SHA256" ]; then
  cat >&2 << EOF
ERROR: apk_sha256_not_in_receipt
  file=$RECEIPT_IN
EOF
  exit 1
fi

# Estratégia 1: Procurar mapping SHA-256 -> commit em docs/BUILDS.md (se existir)
PRODUCER_COMMIT=""
CONFIDENCE=""
CONFIDENCE_SOURCE=""

if [ -f "docs/BUILDS.md" ]; then
  PRODUCER_COMMIT=$(grep "$APK_SHA256" docs/BUILDS.md 2>/dev/null | head -1 | cut -d' ' -f1 || true)
  if [ -n "$PRODUCER_COMMIT" ]; then
    CONFIDENCE="MAPPED"
    CONFIDENCE_SOURCE="docs/BUILDS.md"
  fi
fi

# Estratégia 2: Se não encontrou mapping, usar HEAD de git (best guess)
if [ -z "$PRODUCER_COMMIT" ]; then
  if command -v git > /dev/null 2>&1; then
    PRODUCER_COMMIT=$(git rev-parse HEAD 2>/dev/null || echo UNKNOWN)
    PRODUCER_BRANCH=$(git rev-parse --abbrev-ref HEAD 2>/dev/null || echo UNKNOWN)
    CONFIDENCE="HEAD_ASSUMED"
    CONFIDENCE_SOURCE="git_current_head"
  else
    PRODUCER_COMMIT="UNKNOWN"
    CONFIDENCE="UNRESOLVED"
    CONFIDENCE_SOURCE="git_not_available"
  fi
else
  PRODUCER_BRANCH=$(git log -1 --format='%d' "$PRODUCER_COMMIT" 2>/dev/null | tr -d ' ()' || echo UNKNOWN)
fi

# Gerar linked receipt
RECEIPT_DIR="${HOME}/.rafcodephi/receipts"
mkdir -p "$RECEIPT_DIR"
RECEIPT_BASE=$(date -u +%Y%m%dT%H%M%SZ 2>/dev/null || echo unset)
LINKED_RECEIPT="${RECEIPT_DIR}/apk_linked_${RECEIPT_BASE}.receipt"

TIMESTAMP=$(date -u +%Y-%m-%dT%H:%M:%SZ 2>/dev/null || echo UNSET)

# Calcular hash da cadeia (para integridade do receipt)
RECEIPT_CONTENT_HASH=$(sha256sum "$RECEIPT_IN" 2>/dev/null | cut -d' ' -f1)

# Escrever linked receipt (append-only)
{
  echo "=== SOURCE RECEIPT ==="
  cat "$RECEIPT_IN"
  echo ""
  echo "=== LINKAGE METADATA ==="
  echo "source_receipt=$RECEIPT_IN"
  echo "source_receipt_hash=$RECEIPT_CONTENT_HASH"
  echo "apk_sha256=$APK_SHA256"
  echo "producer_commit=$PRODUCER_COMMIT"
  echo "producer_branch=${PRODUCER_BRANCH:-unknown}"
  echo "confidence=$CONFIDENCE"
  echo "confidence_source=$CONFIDENCE_SOURCE"
  echo "linked_timestamp=$TIMESTAMP"
  echo "link_tool_version=R2_20260806"
} | tee "$LINKED_RECEIPT"

# Emitir status
echo ""
echo "=== R2 Receipt Linkage Complete ==="
echo "linked_receipt=$LINKED_RECEIPT"
echo "linked_size=$(stat -c%s "$LINKED_RECEIPT" 2>/dev/null || echo UNKNOWN)"
echo "linked_hash=$(sha256sum "$LINKED_RECEIPT" 2>/dev/null | cut -d' ' -f1)"
echo "confidence=$CONFIDENCE"

exit 0
