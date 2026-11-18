#!/usr/bin/env bash
# Build tools belong to the hosted build boundary; the bit kernel remains Java.
set -euo pipefail
root=${1:-.}
out=${2:?usage: verify_java_bit_android.sh ROOT NEW_OUTPUT_DIRECTORY}
mkdir "$out" # Refuse to reuse stale output.
out=$(cd "$out" && pwd)
mkdir "$out/classes" "$out/dex"
src="$root/app/src/main/java/com/termux/rafacodephi/lowlevel"
javac --release 8 -Xlint:all,-options -Werror -d "$out/classes" \
  "$src/AuthorialBitOps.java" "$src/AuthorialBitOpsAdversarial.java" \
  "$root/tools/lowlevel/java/BitOpsBenchmark.java"
java -cp "$out/classes" com.termux.rafacodephi.lowlevel.AuthorialBitOpsAdversarial
mapfile -t classes < <(find "$out/classes" -name '*.class' -print | sort)
"${D8:-d8}" --min-api 21 --output "$out/dex" "${classes[@]}"
python3 "$root/scripts/verify_java_bit_dex.py" "$out/dex/classes.dex" > "$out/dex-receipt.json"
python3 "$root/tests/test_java_bit_dex.py" "$out/dex/classes.dex"
# A DEX JAR can be run with dalvikvm in an authorized Termux session; no install.
python3 - "$out" <<'PY'
from pathlib import Path
import sys, zipfile
p=Path(sys.argv[1])
with zipfile.ZipFile(p/'bit-core-device.jar','w',zipfile.ZIP_STORED) as z:
    z.write(p/'dex/classes.dex','classes.dex')
PY
sha256sum "$out/dex/classes.dex" "$out/bit-core-device.jar" > "$out/artifact-sha256.txt"
printf '%s\n' 'DEX_BUILD_AND_API_PASS_SCOPED; ANDROID_RUNTIME=TOKEN_VAZIO_NOT_RUN'
