#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
OUT_DIR="${RAFCODEPHI_AUDITOR_OUT_DIR:-${ROOT_DIR}/out/rafcodephi-auditor}"
METRICS_TSV="${OUT_DIR}/metrics.tsv"
REPORT_MD="${OUT_DIR}/auditor-report.md"
REPORT_JSON="${OUT_DIR}/auditor-report.json"
HISTORY_JSONL="${OUT_DIR}/auditor-history.jsonl"
RUN_ID="${RAFCODEPHI_AUDITOR_RUN_ID:-$(date -u +%Y%m%dT%H%M%SZ)}"
START_UTC="$(date -u +%Y-%m-%dT%H:%M:%SZ)"
DEFAULT_MODULES=(inventory cpu memory storage git clang busybox proot report)

mkdir -p "${OUT_DIR}"
printf 'family\tmetric\tvalue\tunit\tstatus\tevidence\n' > "${METRICS_TSV}"

log() { printf '[rafcodephi-auditor] %s\n' "$*"; }

have() { command -v "$1" >/dev/null 2>&1; }

now_ms() {
  if have python3; then
    python3 - <<'PY'
import time
print(int(time.time() * 1000))
PY
  else
    date +%s000
  fi
}

escape_tsv() {
  printf '%s' "${1:-}" | tr '\t\n\r' '   '
}

metric() {
  local family="$1" metric_name="$2" value="$3" unit="$4" status="$5" evidence="$6"
  printf '%s\t%s\t%s\t%s\t%s\t%s\n' \
    "$(escape_tsv "$family")" \
    "$(escape_tsv "$metric_name")" \
    "$(escape_tsv "$value")" \
    "$(escape_tsv "$unit")" \
    "$(escape_tsv "$status")" \
    "$(escape_tsv "$evidence")" >> "${METRICS_TSV}"
}

elapsed_ms() {
  local start="$1" end
  end="$(now_ms)"
  printf '%s' "$((end - start))"
}

module_inventory() {
  log "inventory"
  metric inventory run_id "${RUN_ID}" text ok "auditor execution id"
  metric inventory generated_at_utc "${START_UTC}" iso8601 ok "UTC timestamp"
  metric inventory os "$(uname -s 2>/dev/null || echo unknown)" text ok "uname -s"
  metric inventory kernel "$(uname -r 2>/dev/null || echo unknown)" text ok "uname -r"
  metric inventory machine "$(uname -m 2>/dev/null || echo unknown)" text ok "uname -m"

  if have getconf; then
    metric inventory page_size "$(getconf PAGESIZE 2>/dev/null || echo unknown)" bytes ok "getconf PAGESIZE"
  else
    metric inventory page_size unknown bytes skipped "getconf unavailable"
  fi

  if have nproc; then
    metric inventory processors "$(nproc 2>/dev/null || echo unknown)" count ok "nproc"
  elif [ -f /proc/cpuinfo ]; then
    metric inventory processors "$(grep -c '^processor' /proc/cpuinfo 2>/dev/null || echo unknown)" count ok "/proc/cpuinfo"
  else
    metric inventory processors unknown count skipped "processor count unavailable"
  fi

  if [ -f /proc/cpuinfo ]; then
    local cpu_model features neon_state
    cpu_model="$(grep -m1 -E 'Hardware|model name|Processor' /proc/cpuinfo 2>/dev/null | cut -d: -f2- | sed 's/^ *//' || true)"
    features="$(grep -m1 -E 'Features|flags' /proc/cpuinfo 2>/dev/null | cut -d: -f2- | sed 's/^ *//' || true)"
    neon_state=absent
    printf '%s' "$features" | grep -Eqi 'neon|asimd' && neon_state=present
    metric inventory cpu_model "${cpu_model:-unknown}" text ok "/proc/cpuinfo"
    metric inventory simd_neon "${neon_state}" bool ok "/proc/cpuinfo features"
  else
    metric inventory cpu_model unknown text skipped "/proc/cpuinfo unavailable"
    metric inventory simd_neon unknown bool skipped "/proc/cpuinfo unavailable"
  fi

  if [ -r /proc/meminfo ]; then
    local mem_kb
    mem_kb="$(awk '/MemTotal:/ {print $2; exit}' /proc/meminfo 2>/dev/null || echo unknown)"
    metric inventory mem_total_kb "${mem_kb}" KiB ok "/proc/meminfo"
  else
    metric inventory mem_total_kb unknown KiB skipped "/proc/meminfo unavailable"
  fi

  if have df; then
    local fs_line
    fs_line="$(df -k . 2>/dev/null | awk 'NR==2 {print $2 ":" $4 ":" $5}' || echo unknown)"
    metric inventory filesystem_current "${fs_line}" total_kb:avail_kb:used_pct ok "df -k ."
  else
    metric inventory filesystem_current unknown text skipped "df unavailable"
  fi
}

module_cpu() {
  log "cpu"
  if ! have awk; then
    metric cpu awk_sqrt_loop skipped ms skipped "awk unavailable"
    return
  fi
  local start elapsed result
  start="$(now_ms)"
  result="$(awk 'BEGIN{s=0; for (i=1; i<=200000; i++) s += sqrt(i); printf "%.3f", s}' 2>/dev/null || echo error)"
  elapsed="$(elapsed_ms "$start")"
  if [ "$result" = error ]; then
    metric cpu awk_sqrt_loop error ms failed "awk sqrt loop failed"
  else
    metric cpu awk_sqrt_loop "${elapsed}" ms ok "200k sqrt loop checksum=${result}"
  fi
}

module_memory() {
  log "memory"
  if ! have dd; then
    metric memory zero_to_null skipped MBps skipped "dd unavailable"
    return
  fi
  local start elapsed bytes mbps
  bytes=$((64 * 1024 * 1024))
  start="$(now_ms)"
  dd if=/dev/zero of=/dev/null bs=1M count=64 >/dev/null 2>&1 || {
    metric memory zero_to_null error MBps failed "dd zero to null failed"
    return
  }
  elapsed="$(elapsed_ms "$start")"
  if [ "${elapsed}" -le 0 ]; then elapsed=1; fi
  mbps=$((bytes * 1000 / elapsed / 1024 / 1024))
  metric memory zero_to_null "${mbps}" MBps ok "64MiB /dev/zero -> /dev/null elapsed_ms=${elapsed}"
}

module_storage() {
  log "storage"
  if ! have dd; then
    metric storage sequential_write skipped MBps skipped "dd unavailable"
    metric storage sequential_read skipped MBps skipped "dd unavailable"
    return
  fi
  local probe bytes start elapsed write_mbps read_mbps
  probe="${OUT_DIR}/storage-probe.bin"
  bytes=$((4 * 1024 * 1024))

  start="$(now_ms)"
  dd if=/dev/zero of="${probe}" bs=1M count=4 conv=fsync >/dev/null 2>&1 || {
    rm -f "${probe}"
    metric storage sequential_write error MBps failed "4MiB write probe failed"
    return
  }
  elapsed="$(elapsed_ms "$start")"
  if [ "${elapsed}" -le 0 ]; then elapsed=1; fi
  write_mbps=$((bytes * 1000 / elapsed / 1024 / 1024))
  metric storage sequential_write "${write_mbps}" MBps ok "4MiB write+fsync elapsed_ms=${elapsed}"

  start="$(now_ms)"
  dd if="${probe}" of=/dev/null bs=1M >/dev/null 2>&1 || {
    rm -f "${probe}"
    metric storage sequential_read error MBps failed "4MiB read probe failed"
    return
  }
  elapsed="$(elapsed_ms "$start")"
  if [ "${elapsed}" -le 0 ]; then elapsed=1; fi
  read_mbps=$((bytes * 1000 / elapsed / 1024 / 1024))
  metric storage sequential_read "${read_mbps}" MBps ok "4MiB read elapsed_ms=${elapsed}"
  rm -f "${probe}"
}

module_git() {
  log "git"
  if ! have git; then
    metric git available false bool skipped "git unavailable"
    return
  fi
  metric git available true bool ok "git found"
  if git rev-parse --is-inside-work-tree >/dev/null 2>&1; then
    local start elapsed head
    head="$(git rev-parse --short=12 HEAD 2>/dev/null || echo unknown)"
    metric git head "${head}" sha ok "git rev-parse HEAD"
    start="$(now_ms)"
    git status --short >/dev/null 2>&1 || true
    elapsed="$(elapsed_ms "$start")"
    metric git status_short "${elapsed}" ms ok "git status --short"
  else
    metric git worktree false bool skipped "not inside git worktree"
  fi
}

module_clang() {
  log "clang"
  if ! have clang; then
    metric clang available false bool skipped "clang unavailable"
    return
  fi
  local src bin start elapsed
  src="${OUT_DIR}/clang-probe.c"
  bin="${OUT_DIR}/clang-probe"
  cat > "${src}" <<'C'
#include <stdint.h>
#include <stdio.h>
int main(void) { uint64_t s = 0; for (uint64_t i = 0; i < 100000; ++i) s += i; printf("%llu\n", (unsigned long long)s); return 0; }
C
  start="$(now_ms)"
  if clang -O2 "${src}" -o "${bin}" >/dev/null 2>&1; then
    elapsed="$(elapsed_ms "$start")"
    metric clang tiny_c_compile "${elapsed}" ms ok "clang -O2 tiny C"
    "${bin}" >/dev/null 2>&1 || true
  else
    metric clang tiny_c_compile error ms failed "clang tiny C compile failed"
  fi
  rm -f "${src}" "${bin}"
}

module_busybox() {
  log "busybox"
  if ! have busybox; then
    metric busybox available false bool skipped "busybox unavailable"
    return
  fi
  local version start elapsed
  version="$(busybox 2>/dev/null | head -n1 || echo busybox)"
  metric busybox available true bool ok "${version}"
  start="$(now_ms)"
  busybox sh -c 'i=0; while [ $i -lt 200 ]; do i=$((i+1)); echo $i >/dev/null; done' >/dev/null 2>&1 || true
  elapsed="$(elapsed_ms "$start")"
  metric busybox shell_loop "${elapsed}" ms ok "busybox sh 200 iterations"
}

module_proot() {
  log "proot"
  if ! have proot; then
    metric proot available false bool skipped "proot unavailable"
    return
  fi
  local version start elapsed
  version="$(proot --version 2>/dev/null | head -n1 || echo proot)"
  metric proot available true bool ok "${version}"
  start="$(now_ms)"
  proot /bin/sh -c true >/dev/null 2>&1 || true
  elapsed="$(elapsed_ms "$start")"
  metric proot baseline_exec "${elapsed}" ms ok "proot /bin/sh -c true"
}

json_escape_python() {
  python3 -c 'import json,sys; print(json.dumps(sys.stdin.read()))'
}

module_report() {
  log "report"
  local end_utc
  end_utc="$(date -u +%Y-%m-%dT%H:%M:%SZ)"

  {
    echo "# RAFCODEΦ Auditor Report"
    echo
    echo "> Coerência × Evidência × Utilidade — execução local determinística e segura."
    echo
    echo "| Campo | Valor |"
    echo "|---|---|"
    echo "| Run ID | ${RUN_ID} |"
    echo "| Início UTC | ${START_UTC} |"
    echo "| Fim UTC | ${end_utc} |"
    echo "| Diretório | ${OUT_DIR} |"
    echo
    echo "## Métricas"
    echo
    echo "| Família | Métrica | Valor | Unidade | Estado | Evidência |"
    echo "|---|---|---:|---|---|---|"
    awk -F '\t' 'NR>1 {gsub(/\|/, "\\|", $6); printf "| `%s` | `%s` | %s | %s | %s | %s |\n", $1, $2, $3, $4, $5, $6}' "${METRICS_TSV}"
    echo
    echo "## Leitura operacional"
    echo
    echo "- Medição local, sem root e sem alteração agressiva de governor/scheduler."
    echo "- Benchmarks são leves para preservar aparelhos ARMv7 e ambientes com pouca RAM."
    echo '- Métricas `skipped` indicam ausência de ferramenta ou permissão, não falha estrutural.'
    echo "- Regressão real exige histórico e múltiplas rodadas em condições térmicas comparáveis."
    echo
    echo "## Retroalimentação R3"
    echo
    echo '```text'
    echo "F_ok   = inventário + microbenchmarks + relatório local"
    echo "F_gap  = temperatura/frequência/energia ainda dependem de sysfs e permissões do dispositivo"
    echo "F_next = executar histórico JSONL e comparar medianas por versão"
    echo '```'
  } > "${REPORT_MD}"

  if have python3; then
    python3 - "${METRICS_TSV}" "${REPORT_JSON}" "${HISTORY_JSONL}" "${RUN_ID}" "${START_UTC}" "${end_utc}" <<'PY'
import csv, json, sys
metrics_path, json_path, history_path, run_id, start_utc, end_utc = sys.argv[1:]
with open(metrics_path, newline='', encoding='utf-8') as f:
    metrics = list(csv.DictReader(f, delimiter='\t'))
payload = {
    "schema": "rafcodephi-auditor/v1",
    "run_id": run_id,
    "start_utc": start_utc,
    "end_utc": end_utc,
    "metrics": metrics,
}
with open(json_path, 'w', encoding='utf-8') as f:
    json.dump(payload, f, ensure_ascii=False, indent=2)
    f.write('\n')
with open(history_path, 'a', encoding='utf-8') as f:
    f.write(json.dumps(payload, ensure_ascii=False, separators=(',', ':')) + '\n')
PY
  else
    printf '{"schema":"rafcodephi-auditor/v1","run_id":"%s","start_utc":"%s","end_utc":"%s","metrics_tsv":"%s"}\n' \
      "${RUN_ID}" "${START_UTC}" "${end_utc}" "${METRICS_TSV}" > "${REPORT_JSON}"
    cat "${REPORT_JSON}" >> "${HISTORY_JSONL}"
  fi

  log "report written: ${REPORT_MD}"
  log "json written: ${REPORT_JSON}"
  log "history written: ${HISTORY_JSONL}"
}

run_module() {
  case "$1" in
    inventory) module_inventory ;;
    cpu) module_cpu ;;
    memory) module_memory ;;
    storage) module_storage ;;
    git) module_git ;;
    clang) module_clang ;;
    busybox) module_busybox ;;
    proot) module_proot ;;
    report) module_report ;;
    all) for module in "${DEFAULT_MODULES[@]}"; do run_module "$module"; done ;;
    *)
      printf 'Usage: %s [all|inventory|cpu|memory|storage|git|clang|busybox|proot|report ...]\n' "$0" >&2
      return 2
      ;;
  esac
}

if [ "$#" -eq 0 ]; then
  set -- all
fi

for module in "$@"; do
  run_module "$module"
done
