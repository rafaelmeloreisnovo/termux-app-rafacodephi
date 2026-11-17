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
! grep -Eq '^import[[:space:]]' "$java_file"
! grep -Eq 'java\.(util|lang|math|nio|io)' "$java_file"
grep -q 'no library calls' "$java_file"
printf '%s\n' 'LOWLEVEL_CONTRACT_STATIC_PASS'
