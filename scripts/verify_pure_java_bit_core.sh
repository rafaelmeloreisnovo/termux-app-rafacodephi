#!/usr/bin/env bash
set -euo pipefail
root=${1:-.}
src="$root/app/src/main/java/com/termux/rafacodephi/lowlevel/AuthorialBitOps.java"
testsrc="$root/app/src/main/java/com/termux/rafacodephi/lowlevel/AuthorialBitOpsAdversarial.java"
manifest="$root/configs/lowlevel-selection.v2.json"
test -s "$src" && test -s "$testsrc" && test -s "$manifest"
python3 - "$manifest" <<'PY'
import json, sys
m=json.load(open(sys.argv[1],encoding='utf-8'))
assert m['schema']=='rafcodephi.lowlevel-selection/v2'
assert m['default_backend']=='JAVA_PURE'
assert m['backends']['JAVA_PURE']['native_library_required'] is False
assert m['claim_allowed'] is False
PY
if rg -n '(^[[:space:]]*import[[:space:]]|\bnative\b|System\.loadLibrary|\b(Math|Arrays|Integer|Long|BitSet|ByteBuffer)\s*\.)' "$src"; then
  echo "UNEXPECTED_LIBRARY_OR_NATIVE_REFERENCE"; exit 21
fi
build_dir="$(mktemp -d)"
trap 'rm -r -- "$build_dir"' EXIT
javac -Xlint:all -Werror -d "$build_dir" "$src" "$testsrc"
java -cp "$build_dir" com.termux.rafacodephi.lowlevel.AuthorialBitOpsAdversarial
# Falsifier: one changed parity constant must be detected by the same vectors.
sed 's/0x6996/0x6997/' "$src" > "$build_dir/AuthorialBitOps.java"
javac -d "$build_dir" "$build_dir/AuthorialBitOps.java" "$testsrc"
if java -cp "$build_dir" com.termux.rafacodephi.lowlevel.AuthorialBitOpsAdversarial >/dev/null 2>&1; then
  echo "MUTATION_SURVIVED"; exit 22
fi
printf '%s\n' 'JAVA_PURE_VECTORS_AND_MUTATION_PASS_SCOPED'
