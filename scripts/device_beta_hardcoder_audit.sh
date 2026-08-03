#!/system/bin/sh
# Read-only beta audit for the installed Termux RAFCODEPhi app.
# Mutating package operations are disabled unless RAF_BETA_MUTATING=1.

set -u

PREFIX="${PREFIX:-/data/data/com.termux.rafacodephi/files/usr}"
HOME="${HOME:-/data/data/com.termux.rafacodephi/files/home}"
PATH="$PREFIX/bin:/system/bin:/system/xbin:/apex/com.android.runtime/bin"
export PREFIX HOME PATH

TS="$(date -u +%Y%m%dT%H%M%SZ 2>/dev/null || echo unknown-time)"
ROOT="$HOME/.rafaelia/receipts/termux-beta"
RUN="$ROOT/run-${TS}-$$"
LOG="$RUN/audit.log"
FACTS="$RUN/facts.env"
SUMMARY="$RUN/summary.txt"
mkdir -p "$RUN" || exit 1
: >"$LOG"
: >"$FACTS"

say() { printf '%s\n' "$*" | tee -a "$LOG"; }
fact() { key="$1"; value="$2"; printf '%s=%s\n' "$key" "$value" >>"$FACTS"; say "$key=$value"; }

first4_hex() {
    file="$1"
    if [ ! -r "$file" ]; then echo TOKEN_VAZIO; return; fi
    if command -v od >/dev/null 2>&1; then
        od -An -tx1 -N4 "$file" 2>/dev/null | tr -d ' \n'
    elif [ -x /system/bin/toybox ]; then
        /system/bin/toybox od -An -tx1 -N4 "$file" 2>/dev/null | tr -d ' \n'
    else
        echo TOKEN_VAZIO
    fi
}

classify_binary() {
    name="$1"; file="$PREFIX/bin/$name"
    if [ ! -e "$file" ]; then echo MISSING; return; fi
    if [ ! -x "$file" ]; then echo PRESENT_NOT_EXECUTABLE; return; fi
    magic="$(first4_hex "$file")"
    if [ "$magic" = 7f454c46 ]; then echo ELF; return; fi
    if grep -a -q 'RAFCODEPHI .* bridge' "$file" 2>/dev/null ||
       grep -a -q 'real apt.*not installed' "$file" 2>/dev/null; then
        echo SCRIPT_BRIDGE; return
    fi
    if head -c 2 "$file" 2>/dev/null | grep -q '#!'; then echo SCRIPT; return; fi
    echo OTHER
}

run_probe() {
    key="$1"; shift
    say "=== probe:$key ==="
    "$@" >>"$LOG" 2>&1
    rc=$?
    fact "${key}_exit" "$rc"
    return "$rc"
}

exists_state() { [ -e "$1" ] && echo PRESENT || echo MISSING; }

read_profile() {
    profile_file="$PREFIX/BOOTSTRAP_PROFILE.json"
    if [ -r "$profile_file" ]; then
        value="$(grep -o '"profile"[[:space:]]*:[[:space:]]*"[^"]*"' "$profile_file" 2>/dev/null | head -n1 | sed 's/.*"\([^"]*\)"[[:space:]]*$/\1/')"
        [ -n "$value" ] && { echo "$value"; return; }
    fi
    info_file="$PREFIX/BOOTSTRAP_INFO"
    if [ -r "$info_file" ]; then
        value="$(sed -n 's/^RAFCODEPHI_BOOTSTRAP_PROFILE=//p' "$info_file" | head -n1)"
        [ -n "$value" ] && { echo "$value"; return; }
    fi
    pkg_class="$(classify_binary pkg)"; apt_class="$(classify_binary apt)"
    if [ "$pkg_class" = SCRIPT_BRIDGE ] || [ "$apt_class" = SCRIPT_BRIDGE ]; then
        echo bridge-inferred
    else
        echo TOKEN_VAZIO
    fi
}

say 'RAFCODEPHI TERMUX APP BETA HARDCODER AUDIT'
fact schema rafcodephi-device-beta-audit/v1
fact timestamp_utc "$TS"
fact prefix "$PREFIX"
fact home "$HOME"
fact uid "$(id -u 2>/dev/null || echo TOKEN_VAZIO)"
fact uname "$(uname -m 2>/dev/null || echo TOKEN_VAZIO)"
fact android_sdk "$(getprop ro.build.version.sdk 2>/dev/null || echo TOKEN_VAZIO)"
fact android_release "$(getprop ro.build.version.release 2>/dev/null || echo TOKEN_VAZIO)"
fact device_model "$(getprop ro.product.model 2>/dev/null | tr ' ' '_' || echo TOKEN_VAZIO)"
fact primary_abi "$(getprop ro.product.cpu.abi 2>/dev/null || echo TOKEN_VAZIO)"
fact bootstrap_profile "$(read_profile)"

for name in sh pkg apt apt-get dpkg busybox proot bash curl git python; do
    fact "bin_${name}_class" "$(classify_binary "$name")"
done

fact sources_list "$(exists_state "$PREFIX/etc/apt/sources.list")"
fact dpkg_status "$(exists_state "$PREFIX/var/lib/dpkg/status")"
fact apt_lists "$(exists_state "$PREFIX/var/lib/apt/lists")"
fact apt_cache "$(exists_state "$PREFIX/var/cache/apt/archives")"
fact ssl_certs "$(exists_state "$PREFIX/etc/tls/cert.pem")"
[ -e "$PREFIX/etc/tls/cert.pem" ] || fact ssl_certs_alt "$(exists_state "$PREFIX/etc/ssl/certs")"

run_probe shell_exec "$PREFIX/bin/sh" -c 'printf "shell_exec_ok=1\n"' || true
run_probe pkg_help "$PREFIX/bin/pkg" help || true
run_probe apt_version "$PREFIX/bin/apt" --version || true
if [ -x "$PREFIX/bin/dpkg" ]; then
    run_probe dpkg_version "$PREFIX/bin/dpkg" --version || true
else
    fact dpkg_version_exit TOKEN_VAZIO_MISSING
fi

fact storage_home "$(exists_state "$HOME/storage")"
fact storage_shared "$(exists_state "$HOME/storage/shared")"
fact storage_downloads "$(exists_state "$HOME/storage/downloads")"
if [ -r "$HOME/storage/downloads" ]; then
    fact storage_downloads_readable YES
else
    fact storage_downloads_readable NO_OR_PERMISSION_DENIED
fi

DNS_STATE=TOKEN_VAZIO
if [ -x /system/bin/ping ]; then
    /system/bin/ping -c 1 -W 2 packages.termux.dev >>"$LOG" 2>&1
    [ "$?" = 0 ] && DNS_STATE=PASS_PING || DNS_STATE=FAIL_OR_ICMP_BLOCKED
elif [ -x /system/bin/toybox ]; then
    /system/bin/toybox ping -c 1 -W 2 packages.termux.dev >>"$LOG" 2>&1
    [ "$?" = 0 ] && DNS_STATE=PASS_PING || DNS_STATE=FAIL_OR_ICMP_BLOCKED
fi
fact dns_probe "$DNS_STATE"

CURL_CLASS="$(classify_binary curl)"
if [ "$CURL_CLASS" = ELF ] || [ "$CURL_CLASS" = SCRIPT ]; then
    "$PREFIX/bin/curl" -fsSI --max-time 10 \
        https://packages.termux.dev/apt/termux-main/dists/stable/Release >>"$LOG" 2>&1
    fact tls_repository_head_exit "$?"
else
    fact tls_repository_head_exit TOKEN_VAZIO_CURL_NOT_REAL
fi

PKG_CLASS="$(classify_binary pkg)"
APT_CLASS="$(classify_binary apt)"
APT_GET_CLASS="$(classify_binary apt-get)"
DPKG_CLASS="$(classify_binary dpkg)"
PROFILE_STATE=INCOMPLETE_DEVICE_OBSERVED
if [ "$PKG_CLASS" = SCRIPT_BRIDGE ] || [ "$APT_CLASS" = SCRIPT_BRIDGE ] || [ "$APT_GET_CLASS" = SCRIPT_BRIDGE ]; then
    PROFILE_STATE=BRIDGE_ONLY_DEVICE_OBSERVED
elif [ "$APT_CLASS" = ELF ] && [ "$APT_GET_CLASS" = ELF ] && [ "$DPKG_CLASS" = ELF ] &&
     [ -s "$PREFIX/etc/apt/sources.list" ] && [ -e "$PREFIX/var/lib/dpkg/status" ]; then
    PROFILE_STATE=REAL_PKG_STRUCTURAL_DEVICE_OBSERVED
fi

fact structural_state "$PROFILE_STATE"
fact mutating_pkg_test_requested "${RAF_BETA_MUTATING:-0}"
if [ "${RAF_BETA_MUTATING:-0}" = 1 ]; then
    say '=== mutating gate explicitly enabled ==='
    "$PREFIX/bin/pkg" update -y >>"$LOG" 2>&1
    fact pkg_update_exit "$?"
else
    fact pkg_update_exit TOKEN_VAZIO_NOT_REQUESTED
fi
fact claim_allowed false
fact release_allowed false

{
    echo 'RAFCODEPHI TERMUX APP BETA AUDIT'
    echo "run=$RUN"
    echo "structural_state=$PROFILE_STATE"
    echo 'claim_allowed=false'
    echo 'release_allowed=false'
    echo
    echo 'F_ok:'
    grep -E '^(shell_exec_exit|pkg_help_exit|storage_|bin_sh_class|bin_pkg_class)=' "$FACTS" 2>/dev/null || true
    echo
    echo 'F_gap:'
    grep -E '^(bin_apt_class|bin_apt-get_class|bin_dpkg_class|sources_list|dpkg_status|ssl_certs|dns_probe|tls_repository_head_exit|pkg_update_exit)=' "$FACTS" 2>/dev/null || true
    echo
    echo 'F_next=build real-pkg profile, reinstall exact APK, rerun audit, then explicitly run mutating pkg gate'
} >"$SUMMARY"

HASHER=
if command -v sha256sum >/dev/null 2>&1; then HASHER="$(command -v sha256sum)";
elif [ -x /system/bin/sha256sum ]; then HASHER=/system/bin/sha256sum; fi
[ -z "$HASHER" ] || "$HASHER" "$FACTS" "$LOG" "$SUMMARY" >"$RUN/SHA256SUMS"

cat "$SUMMARY"
echo "receipt_dir=$RUN"
