#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
OUT_DIR="${ROOT_DIR}/dist/vectra-benchmarks"
APK_ROOT="${ROOT_DIR}/app/build/outputs/apk"
MATRIX_DIR="${ROOT_DIR}/dist/apk-matrix"
NOW_UTC="$(date -u +%Y-%m-%dT%H:%M:%SZ)"
COMMIT_SHA="$(git -C "${ROOT_DIR}" rev-parse HEAD 2>/dev/null || echo unknown)"

mkdir -p "${OUT_DIR}"

REPORT_MD="${OUT_DIR}/vectra_benchmark_report.md"
RESULT_JSON="${OUT_DIR}/vectra_benchmark_results.json"
RESULT_CSV="${OUT_DIR}/vectra_benchmark_results.csv"
SIZE_TSV="${OUT_DIR}/APK_SIZE_REPORT.tsv"
NATIVE_TSV="${OUT_DIR}/native_libs.tsv"
ELF_TSV="${OUT_DIR}/elf_alignment_report.tsv"
ENV_TXT="${OUT_DIR}/run_environment.txt"
SHA_FILE="${OUT_DIR}/SHA256SUMS.txt"

have() { command -v "$1" >/dev/null 2>&1; }
bytes_of() { stat -c '%s' "$1" 2>/dev/null || stat -f '%z' "$1" 2>/dev/null || echo 0; }

printf 'generated_at_utc=%s\n' "${NOW_UTC}" > "${ENV_TXT}"
printf 'commit_sha=%s\n' "${COMMIT_SHA}" >> "${ENV_TXT}"
printf 'root_dir=%s\n' "${ROOT_DIR}" >> "${ENV_TXT}"
printf 'uname=%s\n' "$(uname -a 2>/dev/null || echo unknown)" >> "${ENV_TXT}"
printf 'java=%s\n' "$(java -version 2>&1 | head -n 1 || echo missing)" >> "${ENV_TXT}"
printf 'gradle_wrapper=%s\n' "$([[ -x "${ROOT_DIR}/gradlew" ]] && echo present || echo missing)" >> "${ENV_TXT}"
printf 'adb=%s\n' "$(have adb && adb version | head -n 1 || echo missing)" >> "${ENV_TXT}"
printf 'readelf=%s\n' "$(have readelf && readelf --version | head -n 1 || echo missing)" >> "${ENV_TXT}"
printf 'sha256sum=%s\n' "$(have sha256sum && echo present || echo missing)" >> "${ENV_TXT}"

printf 'apk\ttype\tabi\tsize_bytes\tpath\n' > "${SIZE_TSV}"
printf 'library\tabi\tsize_bytes\tpath\n' > "${NATIVE_TSV}"
printf 'library\tabi\talignment_0x4000_detected\tload_alignments\tstatus\tpath\n' > "${ELF_TSV}"
printf 'metric,value,unit,status\n' > "${RESULT_CSV}"

apk_count=0
arm64_count=0
arm32_count=0
universal_count=0

scan_apk() {
  local apk="$1"
  local base size abi kind
  base="$(basename "$apk")"
  size="$(bytes_of "$apk")"
  abi="universal"
  [[ "$base" == *"arm64-v8a"* ]] && abi="arm64-v8a"
  [[ "$base" == *"armeabi-v7a"* ]] && abi="armeabi-v7a"
  [[ "$base" == *"x86_64"* ]] && abi="x86_64"
  [[ "$base" == *"x86"* && "$base" != *"x86_64"* ]] && abi="x86"
  kind="unknown"
  [[ "$base" == *"debug"* ]] && kind="debug"
  [[ "$base" == *"release"* ]] && kind="release"

  printf '%s\t%s\t%s\t%s\t%s\n' "$base" "$kind" "$abi" "$size" "$apk" >> "${SIZE_TSV}"
  apk_count=$((apk_count+1))
  [[ "$abi" == "arm64-v8a" ]] && arm64_count=$((arm64_count+1))
  [[ "$abi" == "armeabi-v7a" ]] && arm32_count=$((arm32_count+1))
  [[ "$abi" == "universal" ]] && universal_count=$((universal_count+1))
}

while IFS= read -r -d '' apk; do
  scan_apk "$apk"
done < <(find "${APK_ROOT}" "${MATRIX_DIR}" -type f -name '*.apk' -print0 2>/dev/null || true)

if have sha256sum; then
  ( cd "${ROOT_DIR}" && find app/build/outputs/apk dist/apk-matrix -type f \( -name '*.apk' -o -name '*.so' \) -print0 2>/dev/null | xargs -0 -r sha256sum ) > "${SHA_FILE}" || true
else
  echo 'sha256sum missing' > "${SHA_FILE}"
fi

TMP_UNZIP="${OUT_DIR}/_apk_extract"
rm -rf "${TMP_UNZIP}"
mkdir -p "${TMP_UNZIP}"

first_apk="$(find "${APK_ROOT}" "${MATRIX_DIR}" -type f -name '*.apk' 2>/dev/null | head -n 1 || true)"
if [[ -n "${first_apk}" ]] && have unzip; then
  unzip -q -o "${first_apk}" 'lib/*/*.so' -d "${TMP_UNZIP}" >/dev/null 2>&1 || true
  while IFS= read -r -d '' so; do
    rel="${so#${TMP_UNZIP}/}"
    abi="$(echo "$rel" | awk -F/ '{print $2}')"
    lib="$(basename "$so")"
    size="$(bytes_of "$so")"
    printf '%s\t%s\t%s\t%s\n' "$lib" "$abi" "$size" "$so" >> "${NATIVE_TSV}"
    if have readelf; then
      aligns="$(readelf -l "$so" 2>/dev/null | awk '/LOAD/ {print $NF}' | paste -sd ',' -)"
      if echo "$aligns" | grep -qi '0x4000'; then
        printf '%s\t%s\tyes\t%s\tmeasured\t%s\n' "$lib" "$abi" "$aligns" "$so" >> "${ELF_TSV}"
      else
        printf '%s\t%s\tno\t%s\tmeasured\t%s\n' "$lib" "$abi" "${aligns:-none}" "$so" >> "${ELF_TSV}"
      fi
    else
      printf '%s\t%s\tmissing\tmissing\treadelf_missing\t%s\n' "$lib" "$abi" "$so" >> "${ELF_TSV}"
    fi
  done < <(find "${TMP_UNZIP}/lib" -type f -name '*.so' -print0 2>/dev/null || true)
fi

printf 'apk_count,%s,count,measured\n' "${apk_count}" >> "${RESULT_CSV}"
printf 'apk_arm64_count,%s,count,measured\n' "${arm64_count}" >> "${RESULT_CSV}"
printf 'apk_arm32_count,%s,count,measured\n' "${arm32_count}" >> "${RESULT_CSV}"
printf 'apk_universal_count,%s,count,measured\n' "${universal_count}" >> "${RESULT_CSV}"
printf 'runtime_adb_available,%s,bool,declared\n' "$(have adb && echo true || echo false)" >> "${RESULT_CSV}"
printf 'elf_readelf_available,%s,bool,declared\n' "$(have readelf && echo true || echo false)" >> "${RESULT_CSV}"

cat > "${RESULT_JSON}" <<JSON
{
  "schema": "vectra-grade-benchmark.v1",
  "generated_at_utc": "${NOW_UTC}",
  "commit_sha": "${COMMIT_SHA}",
  "status": {
    "apk_count": ${apk_count},
    "arm64_apk_count": ${arm64_count},
    "arm32_apk_count": ${arm32_count},
    "universal_apk_count": ${universal_count},
    "adb_available": $(have adb && echo true || echo false),
    "readelf_available": $(have readelf && echo true || echo false)
  },
  "artifacts": {
    "markdown": "vectra_benchmark_report.md",
    "csv": "vectra_benchmark_results.csv",
    "apk_size_report": "APK_SIZE_REPORT.tsv",
    "native_libs": "native_libs.tsv",
    "elf_alignment_report": "elf_alignment_report.tsv",
    "sha256": "SHA256SUMS.txt",
    "environment": "run_environment.txt"
  },
  "measurement_policy": {
    "measured": "Gerado nesta execução a partir de arquivos/ferramentas disponíveis.",
    "declared": "Declarado por disponibilidade de ferramenta ou configuração.",
    "missing": "Não medido por ausência de artefato, ADB, readelf ou dispositivo."
  }
}
JSON

{
  echo '# Vectra-grade Benchmark Report'
  echo
  echo "- Generated UTC: ${NOW_UTC}"
  echo "- Commit: ${COMMIT_SHA}"
  echo "- APK count: ${apk_count}"
  echo "- ARM64 APK count: ${arm64_count}"
  echo "- ARM32 APK count: ${arm32_count}"
  echo "- Universal APK count: ${universal_count}"
  echo
  echo '## Status técnico'
  echo
  if [[ "${apk_count}" -gt 0 ]]; then
    echo '- APK artifacts: measured'
  else
    echo '- APK artifacts: missing — rode `./gradlew :app:assembleDebug` ou `./scripts/build_apk_matrix.sh` antes do benchmark completo.'
  fi
  if have readelf; then
    echo '- ELF alignment inspection: available'
  else
    echo '- ELF alignment inspection: missing — `readelf` não encontrado no ambiente.'
  fi
  if have adb; then
    echo '- Runtime/ADB inspection: available, mas este script base ainda não executa teste em dispositivo.'
  else
    echo '- Runtime/ADB inspection: missing — sem ADB/dispositivo, runtime metrics ficam pendentes.'
  fi
  echo
  echo '## Artefatos gerados'
  echo
  echo '- `vectra_benchmark_results.json`'
  echo '- `vectra_benchmark_results.csv`'
  echo '- `APK_SIZE_REPORT.tsv`'
  echo '- `native_libs.tsv`'
  echo '- `elf_alignment_report.tsv`'
  echo '- `SHA256SUMS.txt`'
  echo '- `run_environment.txt`'
  echo
  echo '## F de resolvido'
  echo
  echo 'A trilha industrial já gera artefatos mínimos auditáveis para tamanho, hash, ABI, ambiente e inspeção ELF quando disponível.'
  echo
  echo '## F de gap'
  echo
  echo 'Ainda faltam hooks runtime via ADB e microbenchmarks JNI/C/ASM específicos para medir cold/warm start, shell spawn, JNI overhead, branchless C e NEON em dispositivo real.'
  echo
  echo '## F de next'
  echo
  echo '1. Conectar este script ao GitHub Actions.'
  echo '2. Adicionar binário/hook nativo de microbenchmark RMR.'
  echo '3. Rodar em aparelho Android 15/16 para validar page-size, crash, signal 9/11 e phantom process behavior.'
} > "${REPORT_MD}"

rm -rf "${TMP_UNZIP}"

echo "Vectra-grade benchmark artifacts generated in ${OUT_DIR}"
cat "${REPORT_MD}"
