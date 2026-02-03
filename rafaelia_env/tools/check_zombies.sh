#!/usr/bin/env bash
# RAFAELIA :: zombie/overhead lint (fast, no deps)
set -euo pipefail

root="${1:-.}"

echo "== ZOMBIE CHECK (root=$root) =="

# 1) scripts without strict mode
echo "-- scripts missing 'set -euo pipefail' --"
missing=0
while IFS= read -r -d '' f; do
  if ! grep -q "set -euo pipefail" "$f"; then
    echo "MISSING_STRICT: $f"
    missing=$((missing+1))
  fi
done < <(find "$root" -type f -name '*.sh' -print0)
if [ "$missing" -eq 0 ]; then echo "OK"; fi

# 2) common shell typos
echo "-- suspicious patterns --"
grep -RIn --line-number -E "mkdir\s+[^-].*-p|\b(cd\s+\.\.;\s*){3,}|\bhead\s+-n\s+0\b" "$root" || echo "OK"

# 3) duplicated big docs index (optional)
