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
model="pending"; abi="pending"; abilist="pending"; sdk="pending"; release="pending"; page_size="pending"
install_status="pending"; launch_status="pending"; run_as_status="pending"; final_status="DEVICE_PENDING"
process_list=""; zombie_notes="pending"; pkg_dump_status="pending"; activity_dump_status="pending"
if ! command -v adb >/dev/null 2>&1; then
  echo "adb_not_found" > reports/device_runtime_logcat.txt
else
  dev="$(adb devices | awk 'NR>1 && $2=="device"{print $1; exit}')"
  if [[ -z "$dev" ]]; then
    echo "no_connected_device" > reports/device_runtime_logcat.txt
  else
    model="$(adb shell getprop ro.product.model | tr -d '\r')"
    abi="$(adb shell getprop ro.product.cpu.abi | tr -d '\r')"
    abilist="$(adb shell getprop ro.product.cpu.abilist | tr -d '\r')"
    sdk="$(adb shell getprop ro.build.version.sdk | tr -d '\r')"
    release="$(adb shell getprop ro.build.version.release | tr -d '\r')"
    page_size="$(adb shell getconf PAGE_SIZE 2>/dev/null | tr -d '\r' || true)"; page_size="${page_size:-pending}"
    if [[ -n "$APK" && -f "$APK" ]]; then
      adb install -r "$APK" >/tmp/adb_install.out 2>&1 && install_status="ok" || install_status="failed"
      adb shell monkey -p com.termux.rafacodephi -c android.intent.category.LAUNCHER 1 >/tmp/adb_monkey.out 2>&1 && launch_status="ok" || launch_status="failed"
      sleep 4
      adb logcat -d | rg -i 'termux|rafcodephi|com.termux.rafacodephi' > reports/device_runtime_logcat.txt || true
      adb shell dumpsys package com.termux.rafacodephi > reports/device_runtime_dumpsys_package.txt 2>/dev/null && pkg_dump_status="ok" || pkg_dump_status="warning"
      adb shell dumpsys activity > reports/device_runtime_dumpsys_activity.txt 2>/dev/null && activity_dump_status="ok" || activity_dump_status="warning"
      run_as_status="ok"
      adb shell run-as com.termux.rafacodephi ls files/usr/bin >/tmp/adb_runas_ls.out 2>&1 || run_as_status="warning"
      adb shell run-as com.termux.rafacodephi sh -lc 'echo ok' >/tmp/adb_runas_sh.out 2>&1 || run_as_status="warning"
      process_list="$(adb shell ps -A | rg -i 'termux|rafcodephi|com.termux.rafacodephi' || true)"
      zombie_notes="manual_check_required"
      if [[ "$install_status" == "ok" && "$launch_status" == "ok" ]]; then final_status="DEVICE_PARTIAL"; else final_status="DEVICE_FAILED"; fi
      if [[ "$install_status" == "ok" && "$launch_status" == "ok" && "$run_as_status" == "ok" ]]; then final_status="DEVICE_VALIDATED"; fi
    fi
  fi
fi
jq -n \
  --arg timestamp_utc "$TS" --arg apk_tested "${APK:-not_found}" --arg device_model "$model" --arg abi "$abi" --arg abilist "$abilist" \
  --arg sdk "$sdk" --arg android_release "$release" --arg page_size "$page_size" --arg install_status "$install_status" --arg launch_status "$launch_status" \
  --arg run_as_status "$run_as_status" --arg pkg_dump_status "$pkg_dump_status" --arg activity_dump_status "$activity_dump_status" \
  --arg process_list "$process_list" --arg zombie_orphan_notes "$zombie_notes" --arg final_status "$final_status" \
  '{timestamp_utc:$timestamp_utc,apk_tested:$apk_tested,device_model:$device_model,abi:$abi,abilist:$abilist,sdk:$sdk,android_release:$android_release,page_size:$page_size,install_status:$install_status,launch_status:$launch_status,run_as_status:$run_as_status,dumpsys_package:$pkg_dump_status,dumpsys_activity:$activity_dump_status,process_list:$process_list,zombie_orphan_notes:$zombie_orphan_notes,final_status:$final_status}' \
  > reports/device_runtime_smoke.json
cat > reports/device_runtime_smoke.md <<MD
# Device Runtime Smoke
- timestamp_utc: $TS
- apk_tested: ${APK:-not_found}
- device_model: $model
- abi: $abi
- abilist: $abilist
- sdk: $sdk
- android_release: $release
- page_size: $page_size
- install_status: $install_status
- launch_status: $launch_status
- run_as_status: $run_as_status
- dumpsys_package: $pkg_dump_status
- dumpsys_activity: $activity_dump_status
- zombie_orphan_notes: $zombie_notes
- final_status: $final_status
MD
