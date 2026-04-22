#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT_DIR"

echo "[smoke] validating native structure"
python3 scripts/validate_native_structure.py

echo "[smoke] validating side-by-side contract"
python3 scripts/validate_side_by_side_contract.py

echo "[smoke] generating source inventory"
python3 scripts/generate_code_inventory.py


if [[ -f external/linuxkernel.lock ]]; then
  echo "[smoke] kernel pin metadata"
  grep -E '^(REPO_URL|BRANCH|COMMIT)=' external/linuxkernel.lock
fi

echo "[smoke] done"
