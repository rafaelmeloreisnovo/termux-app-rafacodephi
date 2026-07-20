#!/usr/bin/env sh
set -eu

ADB=${ADB:-adb}
PACKAGE=${RAFAELIA_ZERO_PACKAGE:-com.termux.rafacodephi}
COMPONENT="${PACKAGE}/com.termux.app.rafaelia.RafaeliaZeroProbeActivity"
RECEIPT_REMOTE="files/rafaelia-zero/latest.json"
RECEIPT_LOCAL=${RAFAELIA_ZERO_RECEIPT:-build/reports/rafaelia-zero/device-receipt.json}
APK=${1:-}

fail() {
    printf '%s\n' "RAFAELIA_ZERO_DEVICE_PROBE=FAIL: $*" >&2
    exit 1
}

command -v "$ADB" >/dev/null 2>&1 || fail "adb not found"
command -v python3 >/dev/null 2>&1 || fail "python3 not found"

"$ADB" wait-for-device

if [ -n "$APK" ]; then
    [ -f "$APK" ] || fail "APK not found: $APK"
    "$ADB" install -r -t "$APK" >/dev/null || fail "APK install failed"
fi

DEBUGGABLE=$("$ADB" shell run-as "$PACKAGE" sh -c 'printf debug' 2>/dev/null || true)
[ "$DEBUGGABLE" = "debug" ] || fail "run-as unavailable; install a debuggable APK for $PACKAGE"

"$ADB" shell run-as "$PACKAGE" rm -f "$RECEIPT_REMOTE" >/dev/null 2>&1 || true
"$ADB" shell am force-stop "$PACKAGE" >/dev/null 2>&1 || true

START_OUTPUT=$("$ADB" shell am start -W -n "$COMPONENT" 2>&1) || {
    printf '%s\n' "$START_OUTPUT" >&2
    fail "probe activity could not start"
}
printf '%s\n' "$START_OUTPUT"

attempt=0
while [ "$attempt" -lt 30 ]; do
    if "$ADB" shell run-as "$PACKAGE" cat "$RECEIPT_REMOTE" >/dev/null 2>&1; then
        break
    fi
    attempt=$((attempt + 1))
    sleep 1
done
[ "$attempt" -lt 30 ] || fail "receipt did not appear: $RECEIPT_REMOTE"

mkdir -p "$(dirname "$RECEIPT_LOCAL")"
"$ADB" shell run-as "$PACKAGE" cat "$RECEIPT_REMOTE" > "$RECEIPT_LOCAL" 
[ -s "$RECEIPT_LOCAL" ] || fail "captured receipt is empty"

python3 scripts/validate_rafaelia_zero_device_receipt.py "$RECEIPT_LOCAL"

if command -v sha256sum >/dev/null 2>&1; then
    RECEIPT_SHA256=$(sha256sum "$RECEIPT_LOCAL" | awk '{print $1}')
else
    RECEIPT_SHA256=$(python3 - "$RECEIPT_LOCAL" <<'PY'
import hashlib
import pathlib
import sys
print(hashlib.sha256(pathlib.Path(sys.argv[1]).read_bytes()).hexdigest())
PY
)
fi

DEVICE_SERIAL=$("$ADB" get-serialno | tr -d '\r')
DEVICE_FINGERPRINT=$("$ADB" shell getprop ro.build.fingerprint | tr -d '\r')
APK_PATH=$("$ADB" shell pm path "$PACKAGE" | tr -d '\r' | sed -n '1s/^package://p')

printf '%s\n' \
    "RAFAELIA_ZERO_DEVICE_PROBE=PASS" \
    "receipt=$RECEIPT_LOCAL" \
    "receipt_sha256=$RECEIPT_SHA256" \
    "device_serial=$DEVICE_SERIAL" \
    "device_fingerprint=$DEVICE_FINGERPRINT" \
    "installed_apk=$APK_PATH"
