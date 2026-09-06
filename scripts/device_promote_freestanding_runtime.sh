#!/system/bin/sh
# RAFCODEPHI physical-device evidence producer.
# Fixed command surface only; no eval and no free-form package interpolation.
set -u
umask 077

SCHEMA="raf.freestanding-runtime-evidence.v1"
PHASE="${1:-probe}"
case "$PHASE" in
  probe|bootstrap|vectras|full) ;;
  *) echo "usage: $0 [probe|bootstrap|vectras|full]" >&2; exit 64 ;;
esac

PREFIX_DIR="${PREFIX:-/data/data/com.termux.rafacodephi/files/usr}"
case "$PREFIX_DIR" in
  /data/data/*/files/usr|/data/user/0/*/files/usr) ;;
  *) echo "unsafe/unexpected PREFIX: $PREFIX_DIR" >&2; exit 65 ;;
esac
case "$PREFIX_DIR" in
  *[!A-Za-z0-9._/-]*) echo "unsafe character in PREFIX: $PREFIX_DIR" >&2; exit 65 ;;
esac

GATE="$PREFIX_DIR/libexec/rafproot-fs"
EVIDENCE_ROOT="${RAFCODEPHI_EVIDENCE_DIR:-$PREFIX_DIR/var/lib/rafcodephi/evidence}"
mkdir -p "$EVIDENCE_ROOT" || exit 66

NOW="$(date +%s 2>/dev/null || echo 0)"
case "$NOW" in ''|*[!0-9]*) NOW=0;; esac
[ "$NOW" -gt 0 ] || { echo "unable to obtain unix timestamp" >&2; exit 67; }

RAW_ARCH="$(uname -m 2>/dev/null || echo unknown)"
case "$RAW_ARCH" in
  aarch64|arm64*) ARCH="aarch64" ;;
  armv7*|armv8l|armeabi-v7a) ARCH="armv7" ;;
  arm*) ARCH="arm" ;;
  *) ARCH="unknown" ;;
esac

if [ ! -x "$GATE" ]; then
  echo "TOKEN_VAZIO: missing executable gate: $GATE" >&2
  exit 126
fi
if ! command -v sha256sum >/dev/null 2>&1; then
  echo "TOKEN_VAZIO: sha256sum unavailable; cannot bind physical evidence" >&2
  exit 127
fi

GATE_SHA="$(sha256sum "$GATE" | awk '{print $1}')"
case "$GATE_SHA" in
  *[!0-9a-f]*) echo "invalid gate sha256" >&2; exit 68 ;;
esac
[ "${#GATE_SHA}" -eq 64 ] || { echo "invalid gate sha256 length" >&2; exit 68; }

APK_SHA="${RAFCODEPHI_CANDIDATE_APK_SHA256:-}"
APK_BIND_OK=true
case "$APK_SHA" in
  '')
    APK_JSON="null"
    if [ "$PHASE" = "full" ]; then
      APK_BIND_OK=false
      echo "TOKEN_VAZIO: full phase requires RAFCODEPHI_CANDIDATE_APK_SHA256" >&2
    fi
    ;;
  *[!0-9a-f]*) echo "invalid RAFCODEPHI_CANDIDATE_APK_SHA256" >&2; exit 69 ;;
  *)
    [ "${#APK_SHA}" -eq 64 ] || { echo "invalid candidate APK sha256 length" >&2; exit 69; }
    APK_JSON="\"$APK_SHA\""
    ;;
esac

GATE_SHORT="$(printf '%.12s' "$GATE_SHA")"
RECEIPT_ID="${NOW}-${ARCH}-${GATE_SHORT}"
RUN_DIR="$EVIDENCE_ROOT/$RECEIPT_ID"
mkdir -p "$RUN_DIR" || exit 70

# Install stages are themselves executed through the freestanding exec boundary.
INSTALL_RC=0
if [ "$PHASE" = "bootstrap" ] || [ "$PHASE" = "full" ]; then
  "$GATE" --pkg-bootstrap >"$RUN_DIR/pkg-bootstrap.stdout" 2>"$RUN_DIR/pkg-bootstrap.stderr" || INSTALL_RC=$?
fi
if [ "$INSTALL_RC" -eq 0 ] && { [ "$PHASE" = "vectras" ] || [ "$PHASE" = "full" ]; }; then
  "$GATE" --pkg-vectras >"$RUN_DIR/pkg-vectras.stdout" 2>"$RUN_DIR/pkg-vectras.stderr" || INSTALL_RC=$?
fi

run_check() {
  key="$1"
  shift
  out="$RUN_DIR/$key.stdout"
  err="$RUN_DIR/$key.stderr"
  rc=0
  "$@" >"$out" 2>"$err" || rc=$?
  return "$rc"
}

run_check gate_probe "$GATE" --probe; RC_GATE_PROBE=$?
run_check pkg "$GATE" --run pkg list-installed; RC_PKG=$?
run_check proot "$GATE" --run proot --version; RC_PROOT=$?
run_check proot_distro "$GATE" --run proot-distro --help; RC_PROOT_DISTRO=$?
run_check ninja "$GATE" --run ninja --version; RC_NINJA=$?
run_check clang "$GATE" --run clang --version; RC_CLANG=$?
run_check cmake "$GATE" --run cmake --version; RC_CMAKE=$?

QEMU_REQUIRED=false
RC_QEMU_X86_64=-1
RC_QEMU_IMG=-1
if [ "$PHASE" = "vectras" ] || [ "$PHASE" = "full" ]; then
  QEMU_REQUIRED=true
  run_check qemu_system_x86_64 "$GATE" --run qemu-system-x86_64 --version; RC_QEMU_X86_64=$?
  run_check qemu_img "$GATE" --run qemu-img --version; RC_QEMU_IMG=$?
fi

ALL_OK=true
[ "$INSTALL_RC" -eq 0 ] || ALL_OK=false
[ "$APK_BIND_OK" = "true" ] || ALL_OK=false
for rc in "$RC_GATE_PROBE" "$RC_PKG" "$RC_PROOT" "$RC_PROOT_DISTRO" "$RC_NINJA" "$RC_CLANG" "$RC_CMAKE"; do
  [ "$rc" -eq 0 ] || ALL_OK=false
done
if [ "$QEMU_REQUIRED" = "true" ]; then
  [ "$RC_QEMU_X86_64" -eq 0 ] || ALL_OK=false
  [ "$RC_QEMU_IMG" -eq 0 ] || ALL_OK=false
fi

if [ "$ALL_OK" = "true" ]; then
  RUNTIME_STATE="RUNTIME_PROVEN"
  DEVICE_STATE="DEVICE_PROVEN"
  CLAIM="true"
else
  RUNTIME_STATE="TOKEN_VAZIO"
  DEVICE_STATE="TOKEN_VAZIO"
  CLAIM="false"
fi

check_json() {
  key="$1"
  required="$2"
  rc="$3"
  if [ "$required" = "false" ]; then
    state="NOT_SELECTED"
  elif [ "$rc" -eq 0 ]; then
    state="PASS"
  else
    state="TOKEN_VAZIO"
  fi
  printf '    "%s": {"required": %s, "exit_code": %s, "state": "%s"}' "$key" "$required" "$rc" "$state"
}

RECEIPT="$RUN_DIR/receipt.json"
RECEIPT_TMP="$RUN_DIR/.receipt.json.tmp.$$"
{
  echo '{'
  echo "  \"schema\": \"$SCHEMA\","
  echo "  \"receipt_id\": \"$RECEIPT_ID\","
  echo "  \"created_unix\": $NOW,"
  echo "  \"architecture\": \"$ARCH\","
  echo "  \"prefix\": \"$PREFIX_DIR\","
  echo "  \"gate_sha256\": \"$GATE_SHA\","
  echo "  \"candidate_apk_sha256\": $APK_JSON,"
  echo "  \"phase\": \"$PHASE\","
  echo "  \"runtime_state\": \"$RUNTIME_STATE\","
  echo "  \"device_state\": \"$DEVICE_STATE\","
  echo '  "reproduced_state": "TOKEN_VAZIO",'
  echo "  \"claim_allowed\": $CLAIM,"
  echo '  "checks": {'
  check_json gate_probe true "$RC_GATE_PROBE"; echo ','
  check_json pkg true "$RC_PKG"; echo ','
  check_json proot true "$RC_PROOT"; echo ','
  check_json proot_distro true "$RC_PROOT_DISTRO"; echo ','
  check_json ninja true "$RC_NINJA"; echo ','
  check_json clang true "$RC_CLANG"; echo ','
  check_json cmake true "$RC_CMAKE"; echo ','
  check_json qemu_system_x86_64 "$QEMU_REQUIRED" "$RC_QEMU_X86_64"; echo ','
  check_json qemu_img "$QEMU_REQUIRED" "$RC_QEMU_IMG"; echo
  echo '  }'
  echo '}'
} >"$RECEIPT_TMP" || exit 71
mv "$RECEIPT_TMP" "$RECEIPT" || exit 72

SIDECAR_TMP="$RUN_DIR/.receipt.json.sha256.tmp.$$"
sha256sum "$RECEIPT" >"$SIDECAR_TMP" || exit 73
mv "$SIDECAR_TMP" "$RECEIPT.sha256" || exit 74
printf '%s\n' "$RECEIPT"

if [ "$ALL_OK" = "true" ]; then
  echo "DEVICE_PROVEN: $RECEIPT" >&2
  exit 0
fi
echo "TOKEN_VAZIO: one or more required physical checks or bindings failed; receipt retained: $RECEIPT" >&2
exit 1
