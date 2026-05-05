#!/usr/bin/env bash
set -euo pipefail

PKG_NAME="${1:-com.termux}"
OUT_DIR="dist/runtime-smoke/$(date -u +%Y%m%dT%H%M%SZ)-process"
mkdir -p "$OUT_DIR"

BEFORE="$OUT_DIR/process_before.txt"
AFTER="$OUT_DIR/process_after.txt"
DIFF="$OUT_DIR/process_diff.txt"
REPORT="$OUT_DIR/process_cleanup_report.md"

adb shell ps -A | tee "$BEFORE" >/dev/null

echo "Open one terminal session, run: echo process_probe_ok ; exit" >&2
sleep 15

adb shell ps -A | tee "$AFTER" >/dev/null

diff -u "$BEFORE" "$AFTER" > "$DIFF" || true

ZOMBIES=$(adb shell "ps -A -o USER,PID,PPID,STAT,NAME 2>/dev/null | grep ' Z' || true")

{
  echo "# Process Cleanup Probe"
  echo "- Package: $PKG_NAME"
  echo "- Before snapshot: $(basename "$BEFORE")"
  echo "- After snapshot: $(basename "$AFTER")"
  echo "- Diff: $(basename "$DIFF")"
  if [[ -n "$ZOMBIES" ]]; then
    echo "- Zombie candidates detected: yes"
    echo '```'
    echo "$ZOMBIES"
    echo '```'
  else
    echo "- Zombie candidates detected: no"
  fi
} > "$REPORT"

echo "Process cleanup probe report: $REPORT"
