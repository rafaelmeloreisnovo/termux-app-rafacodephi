#!/usr/bin/env bash
set -euo pipefail
root=${1:-.}
manifest="$root/configs/lowlevel-selection.v1.json"
java_file="$root/app/src/main/java/com/termux/rafacodephi/lowlevel/AuthorialBitOps.java"
vectors="$root/app/src/main/java/com/termux/rafacodephi/lowlevel/AuthorialBitOpsVectors.java"
license="$root/LICENSE-RAFCODEPHI-STUDY-RESEARCH.md"
for f in "$manifest" "$java_file" "$vectors" "$license"; do test -f "$f" || { echo "MISSING:$f"; exit 20; }; done
grep -q '"claim_allowed": false' "$manifest"
grep -q 'PURE_CORE' "$manifest"; grep -q 'PLATFORM_GATE' "$manifest"; grep -q 'HOSTED_BOUNDARY' "$manifest"
# Inspect the actual implementation; prose changes are not a semantic gate.
if grep -Eq '^[[:space:]]*import[[:space:]]|java\.(util|lang|math|nio|io)|System\.loadLibrary|^[[:space:]]*public[[:space:]]+static[[:space:]]+native' "$java_file"; then
  echo 'UNEXPECTED_HOSTED_OR_NATIVE_DEPENDENCY'; exit 21
fi
printf '%s\n' 'LOWLEVEL_CONTRACT_STATIC_PASS'
