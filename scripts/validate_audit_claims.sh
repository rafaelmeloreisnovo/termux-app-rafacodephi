#!/usr/bin/env bash
set -euo pipefail
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT_DIR"
mkdir -p reports
FORBIDDEN='ISO certified|certificado ISO|ISO compliant|compliance ISO garantido|conforme ISO|certificação ISO'
if rg -n "$FORBIDDEN" . --glob '!reports/**' --glob '!.git/**' --glob '!docs/AUDIT_CLAIMS_POLICY.md' --glob '!scripts/validate_audit_claims.sh'; then
  echo "Forbidden audit claim found" >&2
  exit 1
fi
cat > reports/audit_claims_validation.md <<EOF
# Audit claims validation

Status: PASS

No forbidden ISO certification claims were found.
EOF
