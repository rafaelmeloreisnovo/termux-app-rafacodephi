#!/usr/bin/env bash
set -euo pipefail
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT_DIR"
mkdir -p reports
APK="${1:-}"
if [[ -z "$APK" ]]; then
  for g in dist/apk-matrix/signed/*.apk dist/apk-matrix/unsigned/*.apk app/build/outputs/apk/debug/*.apk; do [[ -f "$g" ]] && APK="$g" && break; done
fi
TS="$(date -u +%Y-%m-%dT%H:%M:%SZ)"
status="DEVICE_PENDING"; install_status="pending"; launch_status="pending"; runas_status="pending"
if ! command -v adb >/dev/null 2>&1; then
  echo "adb not found" > reports/device_runtime_logcat.txt
else
  dev="$(adb devices | awk 'NR>1 && $2=="device"{print $1;exit}')"
  if [[ -n "${dev:-}" && -n "$APK" && -f "$APK" ]]; then
    model="$(adb shell getprop ro.product.model | tr -d '\r')"; abi="$(adb shell getprop ro.product.cpu.abi | tr -d '\r')"
    abilist="$(adb shell getprop ro.product.cpu.abilist | tr -d '\r')"; sdk="$(adb shell getprop ro.build.version.sdk | tr -d '\r')"
    release="$(adb shell getprop ro.build.version.release | tr -d '\r')"; pagesize="$(adb shell getconf PAGE_SIZE 2>/dev/null | tr -d '\r' || true)"
    adb install -r "$APK" && install_status="ok" || install_status="failed"
    adb shell monkey -p com.termux.rafacodephi -c android.intent.category.LAUNCHER 1 >/tmp/monkey.out 2>&1 && launch_status="ok" || launch_status="failed"
    sleep 4
    adb logcat -d | rg -i 'termux|rafcodephi' > reports/device_runtime_logcat.txt || true
    runas_status="ok"
    adb shell run-as com.termux.rafacodephi ls files/usr/bin >/tmp/runas1.out 2>&1 || runas_status="warning"
    adb shell run-as com.termux.rafacodephi sh -lc 'echo ok' >/tmp/runas2.out 2>&1 || runas_status="warning"
    pslist="$(adb shell ps -A | rg -i 'termux|rafcodephi|com.termux.rafacodephi' || true)"
    [[ "$install_status" == "ok" && "$launch_status" == "ok" ]] && status="DEVICE_PARTIAL"
    [[ "$runas_status" == "ok" ]] && status="DEVICE_VALIDATED"
  fi
fi
cat > reports/device_runtime_smoke.json <<EOF
{"timestamp_utc":"$TS","apk_tested":"${APK}","install_status":"$install_status","launch_status":"$launch_status","run_as_status":"$runas_status","final_status":"$status"}
EOF
cat > reports/device_runtime_smoke.md <<EOF
# Device Runtime Smoke
- timestamp_utc: $TS
- apk_tested: ${APK:-not_found}
- install_status: $install_status
- launch_status: $launch_status
- run_as_status: $runas_status
- final_status: $status
EOF
