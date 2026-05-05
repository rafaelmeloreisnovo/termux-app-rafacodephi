#!/usr/bin/env bash
set -euo pipefail
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT_DIR"
mkdir -p reports
TS="$(date -u +%Y-%m-%dT%H:%M:%SZ)"
apk=$(find dist app/build/outputs/apk -name '*.apk' 2>/dev/null | head -n1 || true)
apk_size=""; sha="pending"; so_size=""
if [[ -n "$apk" && -f "$apk" ]]; then apk_size=$(stat -c%s "$apk"); sha=$(sha256sum "$apk"|awk '{print $1}'); fi
so=$(find app/build/intermediates -name '*.so' 2>/dev/null | head -n1 || true)
[[ -n "$so" && -f "$so" ]] && so_size=$(stat -c%s "$so")
cat > reports/vectra_grade_benchmarks.csv <<EOF
id,category,name,unit,value,status,source,notes
build.clean_time,build_metrics,Clean build time,s,pending,pending,gradle,not measured
binary.apk_size,binary_metrics,APK size,bytes,${apk_size:-pending},$( [[ -n "$apk_size" ]] && echo measured || echo pending),filesystem,artifact inspection
binary.sha256,binary_metrics,APK sha256,hex,$sha,$( [[ "$sha" != "pending" ]] && echo measured || echo pending),sha256sum,artifact inspection
runtime.cold_start,runtime_metrics,Cold start,ms,pending,pending,adb,requires real device
rmr.equivalence,rmr_equivalence_metrics,RMR equivalence,hash,pending,documented,scripts/rmr_equivalence_check.sh,see dedicated report
EOF
cat > reports/vectra_grade_benchmarks.json <<EOF
{"timestamp_utc":"$TS","iso_notice":"Internal checklist only. No ISO certification or formal compliance claim is made.","status":"not_certified","categories":["build_metrics","binary_metrics","runtime_metrics","cpu_metrics","memory_metrics","io_metrics","stability_metrics","jitter_metrics","jni_metrics","rmr_equivalence_metrics","bootstrap_metrics","device_runtime_metrics"]}
EOF
cat > reports/vectra_grade_benchmarks.md <<EOF
# Vectra-grade Benchmarks
- timestamp_utc: $TS
- measured: apk_size/sha256 only when artifacts are present
- documented: rmr equivalence script linkage
- pending: runtime/cpu/memory/io/jitter/jni without live harness/device
- not_certified: ISO references are internal checklist only
EOF
