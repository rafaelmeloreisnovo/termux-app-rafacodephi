#!/usr/bin/env bash
set -euo pipefail

usage() {
  cat <<EOF
Usage: $0 [--wipe-data] <apk_path> [package_name]
Default package_name: com.termux.rafacodephi
EOF
}

WIPE_DATA=0
if [[ "${1:-}" == "--wipe-data" ]]; then
  WIPE_DATA=1
  shift
fi

APK_PATH="${1:-}"
PKG_NAME="${2:-com.termux.rafacodephi}"
[[ -n "$APK_PATH" ]] || { usage; exit 1; }
[[ -f "$APK_PATH" ]] || { echo "APK not found: $APK_PATH"; exit 1; }

OUT_DIR="dist/runtime-smoke/$(date -u +%Y%m%dT%H%M%SZ)"
mkdir -p "$OUT_DIR"

REPORT_TXT="$OUT_DIR/report.txt"
REPORT_MD="$OUT_DIR/report.md"
LOGCAT_OUT="$OUT_DIR/logcat_${PKG_NAME}.txt"
INSTALL_OUT="$OUT_DIR/install.txt"

{
  echo "runtime_smoke_start=$(date -u +%Y-%m-%dT%H:%M:%SZ)"
  echo "apk_path=$APK_PATH"
  echo "package=$PKG_NAME"
} | tee "$REPORT_TXT"

adb start-server
adb get-state | tee -a "$REPORT_TXT"

if [[ "$WIPE_DATA" -eq 1 ]]; then
  echo "Wiping app data for $PKG_NAME" | tee -a "$REPORT_TXT"
  adb shell pm clear "$PKG_NAME" | tee -a "$REPORT_TXT"
else
  echo "Data wipe skipped (safe default)." | tee -a "$REPORT_TXT"
fi

adb install -r "$APK_PATH" | tee "$INSTALL_OUT"

adb logcat -c || true
adb logcat --pid="$(adb shell pidof "$PKG_NAME" || true)" '*:V' > "$LOGCAT_OUT" 2>&1 &
LOGCAT_PID=$!

adb shell monkey -p "$PKG_NAME" -c android.intent.category.LAUNCHER 1 | tee -a "$REPORT_TXT"
sleep 4

cat >> "$REPORT_TXT" <<'EOF'
Manual terminal smoke checklist now:
1) Open terminal
2) echo rafcodephi_beta_ok
3) pwd
4) id
5) uname -a
6) exit 0
7) Repeat open/close cycles (20 then 100)
EOF

sleep 10
kill "$LOGCAT_PID" || true

{
  echo "# Runtime Smoke Report"
  echo "- Start: $(head -n1 "$REPORT_TXT" | cut -d= -f2-)"
  echo "- APK: $APK_PATH"
  echo "- Package: $PKG_NAME"
  echo "- Install log: $(basename "$INSTALL_OUT")"
  echo "- Logcat: $(basename "$LOGCAT_OUT")"
  echo "- Output dir: $OUT_DIR"
  echo "- Result: Pending manual checklist completion on real device"
} > "$REPORT_MD"

echo "Artifacts in $OUT_DIR"
