#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT_DIR"
mkdir -p reports

PACKAGE_NAME="${RAFCODEPHI_PACKAGE_NAME:-com.termux.rafacodephi}"
REQUIRE_REAL_PKG="${REQUIRE_REAL_PKG:-false}"
TS="$(date -u +%Y-%m-%dT%H:%M:%SZ)"
REPORT_JSON="reports/device_pkg_smoke.json"
REPORT_MD="reports/device_pkg_smoke.md"
REPORT_LOG="reports/device_pkg_smoke.log"

status="DEVICE_PENDING"
reason="pending"
model="pending"
abi="pending"
sdk="pending"
data_dir="pending"
canonical_prefix="/data/data/${PACKAGE_NAME}/files/usr"
runtime_prefix="pending"
prefix_alias_state="pending"
minimal_exit="pending"
proot_exit="pending"
local_repo_exit="pending"
real_pkg_exit="not_requested"

write_reports() {
  jq -n \
    --arg timestamp_utc "$TS" \
    --arg package_name "$PACKAGE_NAME" \
    --arg device_model "$model" \
    --arg abi "$abi" \
    --arg sdk "$sdk" \
    --arg data_dir "$data_dir" \
    --arg canonical_prefix "$canonical_prefix" \
    --arg runtime_prefix "$runtime_prefix" \
    --arg prefix_alias_state "$prefix_alias_state" \
    --arg require_real_pkg "$REQUIRE_REAL_PKG" \
    --arg minimal_exit "$minimal_exit" \
    --arg proot_exit "$proot_exit" \
    --arg local_repo_exit "$local_repo_exit" \
    --arg real_pkg_exit "$real_pkg_exit" \
    --arg final_status "$status" \
    --arg reason "$reason" \
    '{timestamp_utc:$timestamp_utc,package_name:$package_name,device_model:$device_model,abi:$abi,sdk:$sdk,data_dir:$data_dir,canonical_prefix:$canonical_prefix,runtime_prefix:$runtime_prefix,prefix_alias_state:$prefix_alias_state,require_real_pkg:$require_real_pkg,minimal_exit:$minimal_exit,proot_exit:$proot_exit,local_repo_exit:$local_repo_exit,real_pkg_exit:$real_pkg_exit,final_status:$final_status,reason:$reason,claim_allowed_pkg_runtime:($final_status=="DEVICE_REAL_PKG_VALIDATED"),claim_allowed_device_runtime:($final_status=="DEVICE_REAL_PKG_VALIDATED")}' \
    > "$REPORT_JSON"

  cat > "$REPORT_MD" <<MD
# Device pkg smoke

- timestamp_utc: $TS
- package_name: $PACKAGE_NAME
- device_model: $model
- abi: $abi
- sdk: $sdk
- data_dir: $data_dir
- canonical_prefix: $canonical_prefix
- runtime_prefix: $runtime_prefix
- prefix_alias_state: $prefix_alias_state
- REQUIRE_REAL_PKG: $REQUIRE_REAL_PKG
- minimal_exit: $minimal_exit
- proot_exit: $proot_exit
- local_repo_exit: $local_repo_exit
- real_pkg_exit: $real_pkg_exit
- final_status: $status
- reason: $reason

See also: `$REPORT_LOG`.
MD
}

if ! command -v adb >/dev/null 2>&1; then
  echo "adb_not_found" > "$REPORT_LOG"
  status="DEVICE_PENDING"
  reason="adb_not_found"
  write_reports
  [[ "$REQUIRE_REAL_PKG" != "true" ]] || exit 1
  exit 0
fi

dev="$(adb devices | awk 'NR>1 && $2=="device"{print $1; exit}')"
if [[ -z "$dev" ]]; then
  echo "no_connected_device" > "$REPORT_LOG"
  status="DEVICE_PENDING"
  reason="no_connected_device"
  write_reports
  [[ "$REQUIRE_REAL_PKG" != "true" ]] || exit 1
  exit 0
fi

model="$(adb shell getprop ro.product.model | tr -d '\r')"
abi="$(adb shell getprop ro.product.cpu.abi | tr -d '\r')"
sdk="$(adb shell getprop ro.build.version.sdk | tr -d '\r')"
data_dir="$(adb shell run-as "$PACKAGE_NAME" pwd 2>/dev/null | tr -d '\r' | tail -n1 || true)"
if [[ -z "$data_dir" ]]; then
  status="DEVICE_FAILED"
  reason="run_as_or_package_unavailable"
  echo "$reason" > "$REPORT_LOG"
  write_reports
  exit 1
fi
runtime_prefix="$data_dir/files/usr"

# The binaries are intentionally source-built for the canonical package prefix.
# Require the Android-assigned filesDir to resolve to the same filesystem target.
runtime_real="$(adb shell run-as "$PACKAGE_NAME" sh -c 'readlink -f "$PWD/files/usr"' 2>/dev/null | tr -d '\r' | tail -n1 || true)"
canonical_real="$(adb shell run-as "$PACKAGE_NAME" sh -c "readlink -f '$canonical_prefix'" 2>/dev/null | tr -d '\r' | tail -n1 || true)"
if [[ -n "$runtime_real" && "$runtime_real" == "$canonical_real" ]]; then
  prefix_alias_state="PASS"
else
  prefix_alias_state="BLOCKED"
  status="DEVICE_PREFIX_LAYOUT_BLOCKED"
  reason="canonical_source_built_prefix_does_not_alias_android_files_dir"
  printf 'runtime_real=%s\ncanonical_real=%s\n' "$runtime_real" "$canonical_real" > "$REPORT_LOG"
  write_reports
  exit 1
fi

cat > /tmp/rafcodephi-device-pkg-smoke.sh <<'DEVICE_SH'
set +e
CANONICAL_PREFIX="/data/data/com.termux.rafacodephi/files/usr"
PREFIX="$CANONICAL_PREFIX"
HOME="${PWD}/files/home"
export PREFIX HOME
export PATH="$PREFIX/bin:/system/bin:/system/xbin:/apex/com.android.runtime/bin"

echo "=== identity ==="
echo "PWD=$PWD"
echo "HOME=$HOME"
echo "PREFIX=$PREFIX"
echo "PATH=$PATH"

fail=0
check(){
  name="$1"
  shift
  echo "=== $name ==="
  "$@"
  code=$?
  echo "exit_$name=$code"
  if [ "$code" != "0" ]; then fail=1; fi
  return "$code"
}

check cat_help cat --help || true
check ls_home ls "$HOME" || true
check clear clear || true

echo "=== grep ==="
grep x /dev/null >/dev/null 2>&1
g=$?
echo "exit_grep=$g"
if [ "$g" != "0" ] && [ "$g" != "1" ]; then fail=1; fi

check pkg_help pkg help || true
check apt_help apt help || true
check proot_real "$PREFIX/bin/proot.real" --version || true
proot_code=$?

repo_code=0
for required in \
  "$PREFIX/etc/apt/rafcodephi-local.list" \
  "$PREFIX/var/lib/rafcodephi/repo/dists/stable/Release"; do
  if [ ! -f "$required" ]; then
    echo "missing_local_repo=$required"
    repo_code=1
  fi
done
echo "exit_local_repo=$repo_code"
if [ "$repo_code" != "0" ]; then fail=1; fi

if [ "$fail" = "0" ]; then
  echo "MINIMAL_PKG_LAYER=PASS"
  exit 0
fi

echo "MINIMAL_PKG_LAYER=FAIL"
exit 1
DEVICE_SH

adb push /tmp/rafcodephi-device-pkg-smoke.sh /data/local/tmp/rafcodephi-device-pkg-smoke.sh >/dev/null
adb shell chmod 755 /data/local/tmp/rafcodephi-device-pkg-smoke.sh >/dev/null

set +e
adb shell run-as "$PACKAGE_NAME" sh /data/local/tmp/rafcodephi-device-pkg-smoke.sh > "$REPORT_LOG" 2>&1
minimal_exit="$?"
set -e
proot_exit="$(sed -n 's/^exit_proot_real=//p' "$REPORT_LOG" | tail -n1)"
local_repo_exit="$(sed -n 's/^exit_local_repo=//p' "$REPORT_LOG" | tail -n1)"
proot_exit="${proot_exit:-missing}"
local_repo_exit="${local_repo_exit:-missing}"

if [[ "$minimal_exit" == "0" ]]; then
  status="DEVICE_LOCAL_PKG_CANDIDATE_VALIDATED"
  reason="minimal_pkg_proot_local_repo_passed"
else
  status="DEVICE_FAILED"
  reason="minimal_pkg_proot_or_local_repo_failed"
fi

if [[ "$REQUIRE_REAL_PKG" == "true" && "$status" == "DEVICE_LOCAL_PKG_CANDIDATE_VALIDATED" ]]; then
  cat > /tmp/rafcodephi-real-pkg-smoke.sh <<'REAL_PKG_SH'
set -e
PREFIX="/data/data/com.termux.rafacodephi/files/usr"
HOME="${PWD}/files/home"
export PREFIX HOME
export PATH="$PREFIX/bin:/system/bin:/system/xbin:/apex/com.android.runtime/bin"

echo "=== real pkg smoke: local hash-bound repository ==="
pkg update -y
pkg install -y nano
nano --version
pkg install -y python
python --version
pkg install -y git
git --version
proot.real --version
REAL_PKG_SH
  adb push /tmp/rafcodephi-real-pkg-smoke.sh /data/local/tmp/rafcodephi-real-pkg-smoke.sh >/dev/null
  adb shell chmod 755 /data/local/tmp/rafcodephi-real-pkg-smoke.sh >/dev/null
  set +e
  adb shell run-as "$PACKAGE_NAME" sh /data/local/tmp/rafcodephi-real-pkg-smoke.sh >> "$REPORT_LOG" 2>&1
  real_pkg_exit="$?"
  set -e
  if [[ "$real_pkg_exit" == "0" ]]; then
    status="DEVICE_REAL_PKG_VALIDATED"
    reason="local_repo_pkg_update_install_nano_python_git_proot_passed"
  else
    status="DEVICE_FAILED"
    reason="real_pkg_local_repo_update_or_install_failed"
  fi
fi

write_reports

if [[ "$REQUIRE_REAL_PKG" == "true" && "$status" != "DEVICE_REAL_PKG_VALIDATED" ]]; then
  echo "REQUIRE_REAL_PKG=true final_status=$status reason=$reason" >&2
  exit 1
fi

[[ "$status" != "DEVICE_FAILED" ]] || exit 1
exit 0
