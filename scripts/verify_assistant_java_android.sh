#!/usr/bin/env bash
# Pure Java core; hosted JDK / D8 / Python are BUILD tools, never core dependencies.
set -euo pipefail
root=${1:-.}
out=${2:?usage: verify_assistant_java_android.sh ROOT NEW_OUTPUT_DIR}
if test -e "$out"; then echo 'OUTPUT_EXISTS_REFUSE_STALE' >&2; exit 24; fi
mkdir "$out"
out=$(cd "$out" && pwd)
mkdir "$out/classes" "$out/dex"
src="$root/app/src/main/java/com/termux/rafacodephi/assistant"
test -s "$src/RafAssistantArenaV1.java"
test -s "$src/RafAssistantArenaV1Adversarial.java"
test -s "$root/tools/assistant/java/RafAssistantArenaBenchmark.java"
# Gate new low-level source, not the hosted test/benchmark helpers.
python3 - "$src/RafAssistantArenaV1.java" <<'PY'
from pathlib import Path
import re, sys
src=Path(sys.argv[1]).read_text(encoding='utf-8')
src=re.sub(r'/\*.*?\*/|//[^\n]*', '', src, flags=re.S)
if re.search(r'\b(import|native|synchronized)\b|System\s*\.|Runtime\s*\.|Thread\s*\.|\bjava\s*\.', src):
    raise SystemExit('FORBIDDEN_RUNTIME_DEPENDENCY')
print('JAVA_SOURCE_BOUNDARY_PASS_SCOPED')
PY
javac --release 8 -Xlint:all,-options -Werror -d "$out/classes" \
  "$src/RafAssistantArenaV1.java" "$src/RafAssistantArenaV1Adversarial.java" \
  "$root/tools/assistant/java/RafAssistantArenaBenchmark.java"
java -cp "$out/classes" com.termux.rafacodephi.assistant.RafAssistantArenaV1Adversarial
java -cp "$out/classes" com.termux.rafacodephi.assistant.RafAssistantArenaBenchmark > "$out/jvm-benchmark.csv"
if test -n "${D8:-}"; then
  d8bin="$D8"
elif test -n "${ANDROID_HOME:-}" && test -x "$ANDROID_HOME/build-tools/35.0.0/d8"; then
  d8bin="$ANDROID_HOME/build-tools/35.0.0/d8"
else
  echo 'DEX_NOT_RUN_BUILD_TOOLS_MISSING; JVM_ONLY_PASS_SCOPED' >&2
  exit 25
fi
test -x "$d8bin" || { echo 'DEX_NOT_RUN_D8_NOT_EXECUTABLE' >&2; exit 26; }
mapfile -t classes < <(find "$out/classes" -name '*.class' -print | sort)
test "${#classes[@]}" -ge 3
"$d8bin" --min-api 21 --output "$out/dex" "${classes[@]}"
python3 "$root/scripts/verify_assistant_arena_dex.py" "$out/dex/classes.dex" > "$out/dex-receipt.json"
python3 "$root/tests/test_assistant_arena_dex.py" "$out/dex/classes.dex"
python3 - "$out" <<'PY'
from pathlib import Path
import sys, zipfile
p=Path(sys.argv[1])
with zipfile.ZipFile(p/'assistant-arena-device.jar', 'w', zipfile.ZIP_STORED) as archive:
    archive.write(p/'dex/classes.dex','classes.dex')
PY
sha256sum "$out/dex/classes.dex" "$out/assistant-arena-device.jar" \
  "$src/RafAssistantArenaV1.java" "$src/RafAssistantArenaV1Adversarial.java" \
  > "$out/sha256-receipt.txt"
printf '%s\n' 'JVM_VECTOR_AND_DEX_API_PASS_SCOPED; DEVICE=TOKEN_VAZIO_NOT_RUN; claim_allowed=false'
