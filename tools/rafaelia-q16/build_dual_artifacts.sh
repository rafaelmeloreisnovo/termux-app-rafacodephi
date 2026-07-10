#!/data/data/com.termux/files/usr/bin/sh
set -eu

# RAFAELIA Q16 dual-artifact validator
# Produces:
#   rafaelia_node          — audit artifact with symbols
#   rafaelia_node.stripped — distribution artifact
# Both must return zero and emit byte-identical output.

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
BUILD_DIR=${1:-rafaelia-q16-build}

"$SCRIPT_DIR/build_rafaelia_q16.sh" "$BUILD_DIR"
cd "$BUILD_DIR"

[ -x rafaelia_node ] || {
    echo "[FALHA] rafaelia_node nao foi produzido." >&2
    exit 2
}

set +e
./rafaelia_node > original.output.txt 2>&1
RUN_STATUS=$?
set -e

cat original.output.txt
printf '\n[*] Codigo de saida original: %s\n' "$RUN_STATUS"

printf '\n[*] Gerando versao stripped...\n'
cp rafaelia_node rafaelia_node.stripped

if command -v llvm-strip >/dev/null 2>&1; then
    llvm-strip --strip-all rafaelia_node.stripped
elif command -v strip >/dev/null 2>&1; then
    strip --strip-all rafaelia_node.stripped
else
    echo "[FALHA] llvm-strip/strip nao encontrado." >&2
    exit 127
fi
chmod 700 rafaelia_node.stripped

set +e
./rafaelia_node.stripped > stripped.output.txt 2>&1
STRIP_STATUS=$?
set -e

cat stripped.output.txt
printf '\n[*] Codigo de saida stripped: %s\n' "$STRIP_STATUS"

if cmp -s original.output.txt stripped.output.txt; then
    OUTPUT_STATUS=0
    echo "[OK] Saidas byte a byte identicas."
else
    OUTPUT_STATUS=1
    echo "[FALHA] Saidas diferentes."
    if command -v diff >/dev/null 2>&1; then
        diff -u original.output.txt stripped.output.txt || true
    fi
fi

ORIGINAL_SIZE=$(wc -c < rafaelia_node)
STRIPPED_SIZE=$(wc -c < rafaelia_node.stripped)
REDUCTION_BYTES=$((ORIGINAL_SIZE - STRIPPED_SIZE))
ORIGINAL_SHA256=$(sha256sum rafaelia_node | awk '{print $1}')
STRIPPED_SHA256=$(sha256sum rafaelia_node.stripped | awk '{print $1}')

if [ "$RUN_STATUS" -eq 0 ] && \
   [ "$STRIP_STATUS" -eq 0 ] && \
   [ "$OUTPUT_STATUS" -eq 0 ]; then
    CONTRACT_PASSED=true
else
    CONTRACT_PASSED=false
fi

cat <<EOF > rafaelia_q16_dual_artifacts.manifest
schema=rafaelia.q16.dual-artifact.v1
architecture=$(uname -m)
original.name=rafaelia_node
original.size_bytes=$ORIGINAL_SIZE
original.sha256=$ORIGINAL_SHA256
original.exit_status=$RUN_STATUS
stripped.name=rafaelia_node.stripped
stripped.size_bytes=$STRIPPED_SIZE
stripped.sha256=$STRIPPED_SHA256
stripped.exit_status=$STRIP_STATUS
reduction_bytes=$REDUCTION_BYTES
outputs_identical=$([ "$OUTPUT_STATUS" -eq 0 ] && echo true || echo false)
contract_passed=$CONTRACT_PASSED
EOF

printf '\n[*] Comparacao de tamanhos:\n'
wc -c rafaelia_node rafaelia_node.stripped
printf '\n[*] SHA-256:\n'
sha256sum rafaelia_node rafaelia_node.stripped
printf '\n[*] Manifesto:\n'
cat rafaelia_q16_dual_artifacts.manifest

if command -v readelf >/dev/null 2>&1; then
    printf '\n[*] DT_NEEDED original:\n'
    readelf -d rafaelia_node | grep NEEDED || echo "[OK] Nenhum DT_NEEDED."
    printf '\n[*] DT_NEEDED stripped:\n'
    readelf -d rafaelia_node.stripped | grep NEEDED || echo "[OK] Nenhum DT_NEEDED."
fi

[ "$RUN_STATUS" -eq 0 ] || exit "$RUN_STATUS"
[ "$STRIP_STATUS" -eq 0 ] || exit "$STRIP_STATUS"
[ "$OUTPUT_STATUS" -eq 0 ] || exit "$OUTPUT_STATUS"

echo "[OK] Contrato binario duplo satisfeito."
exit 0
