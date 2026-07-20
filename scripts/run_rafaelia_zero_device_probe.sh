#!/usr/bin/env sh
set -eu

ADB=${ADB:-adb}
PACKAGE=${RAFAELIA_ZERO_PACKAGE:-com.termux.rafacodephi}
COMPONENT="${PACKAGE}/com.termux.app.rafaelia.RafaeliaZeroProbeActivity"
RECEIPT_REMOTE="files/rafaelia-zero/latest.json"
REPORT_ROOT=${RAFAELIA_ZERO_REPORT_ROOT:-build/reports/rafaelia-zero}
EVIDENCE_ROOT=${RAFAELIA_ZERO_EVIDENCE_ROOT:-${REPORT_ROOT}/evidence}
APK=${1:-}

fail() {
    printf '%s\n' "RAFAELIA_ZERO_DEVICE_PROBE=FAIL: $*" >&2
    exit 1
}

hash_file() {
    python3 - "$1" <<'PY'
import hashlib
import pathlib
import sys
path = pathlib.Path(sys.argv[1])
digest = hashlib.sha256()
with path.open("rb") as stream:
    for block in iter(lambda: stream.read(1024 * 1024), b""):
        digest.update(block)
print(digest.hexdigest())
PY
}

command -v "$ADB" >/dev/null 2>&1 || fail "adb not found"
command -v python3 >/dev/null 2>&1 || fail "python3 not found"
command -v mktemp >/dev/null 2>&1 || fail "mktemp not found"

mkdir -p "$REPORT_ROOT" "$EVIDENCE_ROOT"
STAGING=$(mktemp -d "${REPORT_ROOT}/staging.XXXXXX")
trap 'rm -rf "$STAGING"' EXIT HUP INT TERM
RECEIPT_LOCAL="${STAGING}/receipt.json"
CAPTURE_LOCAL="${STAGING}/capture.json"
TRANSCRIPT_LOCAL="${STAGING}/transcript.txt"
APK_LOCAL="${STAGING}/apk.bin"
: > "$TRANSCRIPT_LOCAL"

record() {
    printf '%s\n' "$*" | tee -a "$TRANSCRIPT_LOCAL"
}

run_recorded() {
    "$@" 2>&1 | tee -a "$TRANSCRIPT_LOCAL"
}

"$ADB" wait-for-device
record "adb_wait_for_device=PASS"

if [ -n "$APK" ]; then
    [ -f "$APK" ] || fail "APK not found: $APK"
    [ -s "$APK" ] || fail "APK is empty: $APK"
    run_recorded "$ADB" install -r -t "$APK" || fail "APK install failed"
    cp "$APK" "$APK_LOCAL"
    record "apk_source=local-install-argument"
else
    record "apk_source=installed-package-capture"
fi

DEBUGGABLE=$("$ADB" shell run-as "$PACKAGE" sh -c 'printf debug' 2>/dev/null || true)
[ "$DEBUGGABLE" = "debug" ] || fail "run-as unavailable; install a debuggable APK for $PACKAGE"
record "run_as_debuggable=PASS"

"$ADB" shell run-as "$PACKAGE" rm -f "$RECEIPT_REMOTE" >/dev/null 2>&1 || true
"$ADB" shell am force-stop "$PACKAGE" >/dev/null 2>&1 || true

START_OUTPUT=$("$ADB" shell am start -W -n "$COMPONENT" 2>&1) || {
    printf '%s\n' "$START_OUTPUT" | tee -a "$TRANSCRIPT_LOCAL" >&2
    fail "probe activity could not start"
}
printf '%s\n' "$START_OUTPUT" | tee -a "$TRANSCRIPT_LOCAL"

attempt=0
while [ "$attempt" -lt 30 ]; do
    if "$ADB" shell run-as "$PACKAGE" cat "$RECEIPT_REMOTE" >/dev/null 2>&1; then
        break
    fi
    attempt=$((attempt + 1))
    sleep 1
done
[ "$attempt" -lt 30 ] || fail "receipt did not appear: $RECEIPT_REMOTE"
record "receipt_poll_attempts=$attempt"

"$ADB" shell run-as "$PACKAGE" cat "$RECEIPT_REMOTE" > "$RECEIPT_LOCAL"
[ -s "$RECEIPT_LOCAL" ] || fail "captured receipt is empty"
python3 scripts/validate_rafaelia_zero_device_receipt.py "$RECEIPT_LOCAL" 2>&1 | tee -a "$TRANSCRIPT_LOCAL"

DEVICE_SERIAL=$("$ADB" get-serialno | tr -d '\r')
DEVICE_FINGERPRINT=$("$ADB" shell getprop ro.build.fingerprint | tr -d '\r')
APK_PATH=$("$ADB" shell pm path "$PACKAGE" | tr -d '\r' | sed -n '1s/^package://p')
CAPTURED_AT_MS=$(python3 - <<'PY'
import time
print(time.time_ns() // 1_000_000)
PY
)

[ -n "$DEVICE_SERIAL" ] || fail "device serial is empty"
[ -n "$DEVICE_FINGERPRINT" ] || fail "device fingerprint is empty"
[ -n "$APK_PATH" ] || fail "installed APK path is empty"

if [ ! -s "$APK_LOCAL" ]; then
    run_recorded "$ADB" pull "$APK_PATH" "$APK_LOCAL" || fail "installed APK capture failed"
fi
[ -s "$APK_LOCAL" ] || fail "captured APK is empty"

RECEIPT_SHA256=$(hash_file "$RECEIPT_LOCAL")
APK_SHA256=$(hash_file "$APK_LOCAL")
record "RAFAELIA_ZERO_DEVICE_PROBE=PASS"
record "receipt_sha256=$RECEIPT_SHA256"
record "apk_sha256=$APK_SHA256"
record "device_serial=$DEVICE_SERIAL"
record "device_fingerprint=$DEVICE_FINGERPRINT"
record "installed_apk=$APK_PATH"
record "captured_at_unix_ms=$CAPTURED_AT_MS"

python3 - "$CAPTURE_LOCAL" "$PACKAGE" "$DEVICE_SERIAL" "$DEVICE_FINGERPRINT" "$APK_PATH" "$CAPTURED_AT_MS" <<'PY'
import json
import pathlib
import sys
path = pathlib.Path(sys.argv[1])
data = {
    "schema": "rafaelia.zero.device.capture.v1",
    "package": sys.argv[2],
    "device_serial": sys.argv[3],
    "device_fingerprint": sys.argv[4],
    "installed_apk_path": sys.argv[5],
    "captured_at_unix_ms": int(sys.argv[6]),
}
path.write_text(json.dumps(data, sort_keys=True, indent=2) + "\n", encoding="utf-8")
PY

SAFE_SERIAL=$(printf '%s' "$DEVICE_SERIAL" | tr -c 'A-Za-z0-9._-' '_')
BUNDLE_DIR="${EVIDENCE_ROOT}/${CAPTURED_AT_MS}-${SAFE_SERIAL}"
[ ! -e "$BUNDLE_DIR" ] || fail "evidence bundle already exists: $BUNDLE_DIR"

python3 scripts/create_rafaelia_zero_device_bundle.py \
    --receipt "$RECEIPT_LOCAL" \
    --capture "$CAPTURE_LOCAL" \
    --apk "$APK_LOCAL" \
    --transcript "$TRANSCRIPT_LOCAL" \
    --output "$BUNDLE_DIR"
python3 scripts/validate_rafaelia_zero_device_bundle.py "$BUNDLE_DIR"

MATRIX_OUTPUT="${EVIDENCE_ROOT}/matrix.json"
set --
for candidate in "${EVIDENCE_ROOT}"/*; do
    [ -d "$candidate" ] || continue
    [ -f "$candidate/manifest.json" ] || continue
    set -- "$@" "$candidate"
done
python3 scripts/validate_rafaelia_zero_device_matrix.py "$@" --output "$MATRIX_OUTPUT"

MANIFEST_SHA256=$(hash_file "$BUNDLE_DIR/manifest.json")
MATRIX_SHA256=$(hash_file "$MATRIX_OUTPUT")
printf '%s\n' \
    "RAFAELIA_ZERO_OPERATIONAL_EVIDENCE=PASS" \
    "bundle=$BUNDLE_DIR" \
    "manifest_sha256=$MANIFEST_SHA256" \
    "matrix=$MATRIX_OUTPUT" \
    "matrix_sha256=$MATRIX_SHA256" \
    "receipt_sha256=$RECEIPT_SHA256" \
    "apk_sha256=$APK_SHA256" \
    "device_serial=$DEVICE_SERIAL" \
    "device_fingerprint=$DEVICE_FINGERPRINT" \
    "installed_apk=$APK_PATH"
