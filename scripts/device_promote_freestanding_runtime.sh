#!/system/bin/sh
# RAFCODEPHI physical-device evidence producer.
# Fixed command surface only; no eval and no free-form package interpolation.
set -u

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
  [0-9a-f][0-9a-f][0-9a-f][0-9a-f][0-9a-f][0-9a-f][0-9a-f][0-9a-f]*) ;;
  *) echo "invalid gate sha256" >&2; exit 68 ;;
esac
[ "${#GATE_SHA}" -eq 64 ] || { echo "invalid gate sha256 length" >&2; exit 68; }

APK_SHA="${RAFCODEPHI_CANDIDATE_APK_SHA256:-}"
case "$APK_SHA" in
  '') APK_JSON="null" ;;
  *[!0-9a-f]*) echo "invalid RAFCODEPHI_CANDIDATE_APK_SHA256" >&2; exit 69 ;;
  *) [ "${#APK_SHA}" -eq 64 ] || { echo "invalid candidate APK sha256 length" >&2; exit 69; }; APK_JSON="\"$APK_SHA\"" ;;
esac

RECEIPT_ID="${NOW}-${ARCH}-${GATE_SHA%%????????????????????????????????????????????????????}"
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
  key="$1"; required="$2"; shift 2
  out="$RUN_DIR/$key.stdout"
  err="$RUN_DIR/$key.stderr"
  rc=0
  "$@" >"$out" 2>"$err" || rc=$?
  state="PASS"
  [ "$rc" -eq 0 ] || state="TOKEN_VAZIO"
  eval "RC_$key=$rc"
  eval "STATE_$key=$state"
  eval "REQ_$key=$required"
}

run_check gate_probe true "$GATE" --probe
run_check pkg true "$GATE" --run pkg list-installed
run_check proot true "$GATE" --run proot --version
run_check proot_distro true "$GATE" --run proot-distro --help
run_check ninja true "$GATE" --run ninja --version
run_check clang true "$GATE" --run clang --version
run_check cmake true "$GATE" --run cmake --version

if [ "$PHASE" = "vectras" ] || [ "$PHASE" = "full" ]; then
  run_check qemu_system_x86_64 true "$GATE" --run qemu-system-x86_64 --version
  run_check qemu_img true "$GATE" --run qemu-img --version
else
  RC_qemu_system_x86_64=-1; STATE_qemu_system_x86_64=NOT_SELECTED; REQ_qemu_system_x86_64=false
  RC_qemu_img=-1; STATE_qemu_img=NOT_SELECTED; REQ_qemu_img=false
fi

ALL_OK=true
[ "$INSTALL_RC" -eq 0 ] || ALL_OK=false
for key in gate_probe pkg proot proot_distro ninja clang cmake qemu_system_x86_64 qemu_img; do
  eval "required=\$REQ_$key"
  eval "state=\$STATE_$key"
  if [ "$required" = "true" ] && [ "$state" != "PASS" ]; then ALL_OK=false; fi
done

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
  eval "required=\$REQ_$key"
  eval "rc=\$RC_$key"
  eval "state=\$STATE_$key"
  printf '    "%s": {"required": %s, "exit_code": %s, "state": "%s"}' "$key" "$required" "$rc" "$state"
}

RECEIPT="$RUN_DIR/receipt.json"
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
  check_json gate_probe; echo ','
  check_json pkg; echo ','
  check_json proot; echo ','
  check_json proot_distro; echo ','
  check_json ninja; echo ','
  check_json clang; echo ','
  check_json cmake; echo ','
  check_json qemu_system_x86_64; echo ','
  check_json qemu_img; echo
  echo '  }'
  echo '}'
} >"$RECEIPT"

sha256sum "$RECEIPT" >"$RECEIPT.sha256"
printf '%s\n' "$RECEIPT"

if [ "$ALL_OK" = "true" ]; then
  echo "DEVICE_PROVEN: $RECEIPT" >&2
  exit 0
fi
echo "TOKEN_VAZIO: one or more required physical checks failed; receipt retained: $RECEIPT" >&2
exit 1
