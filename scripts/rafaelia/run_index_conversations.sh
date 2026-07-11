#!/data/data/com.termux/files/usr/bin/sh
set -eu

usage() {
  echo "usage: $0 INPUT_JSON OUTPUT_DIR CONVSCAN_BIN" >&2
  exit 64
}

[ "$#" -eq 3 ] || usage
INPUT=$1
OUT=$2
BIN=$3

[ -f "$INPUT" ] || { echo "input not found: $INPUT" >&2; exit 66; }
[ -x "$BIN" ] || { echo "scanner not executable: $BIN" >&2; exit 69; }

mkdir -p "$OUT"
LOCK="$OUT/.index_conversations.lock"
REPORT_TMP="$OUT/source.manifest.json.tmp"
REPORT="$OUT/source.manifest.json"
AUDIT="$OUT/audit.jsonl"
CHECKPOINT="$OUT/checkpoint.state"

if ! mkdir "$LOCK" 2>/dev/null; then
  echo "job already running: $LOCK" >&2
  exit 75
fi
trap 'rmdir "$LOCK" 2>/dev/null || true' EXIT HUP INT TERM

SIZE=$(wc -c < "$INPUT" | tr -d ' ')
START=$(date +%s)
printf '{"event":"START","operation":"index_conversations","input":"%s","size_bytes":%s,"time":%s}\n' \
  "$INPUT" "$SIZE" "$START" >> "$AUDIT"

if "$BIN" "$INPUT" > "$REPORT_TMP"; then
  mv "$REPORT_TMP" "$REPORT"
  END=$(date +%s)
  printf 'state=VERIFIED\ninput=%s\nsize_bytes=%s\ncompleted_at=%s\n' \
    "$INPUT" "$SIZE" "$END" > "$CHECKPOINT"
  printf '{"event":"COMPLETE","state":"VERIFIED","manifest":"%s","time":%s}\n' \
    "$REPORT" "$END" >> "$AUDIT"
  echo "[VERIFIED] $REPORT"
else
  STATUS=$?
  rm -f "$REPORT_TMP"
  END=$(date +%s)
  printf 'state=CONTRADICTION\ninput=%s\nsize_bytes=%s\nerror_code=%s\nfailed_at=%s\n' \
    "$INPUT" "$SIZE" "$STATUS" > "$CHECKPOINT"
  printf '{"event":"FAIL","state":"CONTRADICTION","error_code":%s,"time":%s}\n' \
    "$STATUS" "$END" >> "$AUDIT"
  exit "$STATUS"
fi
