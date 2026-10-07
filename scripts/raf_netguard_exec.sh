#!/data/data/com.termux/files/usr/bin/sh
set -eu

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
OUT="${RAF_NETGUARD_OUT:-$ROOT/build/raf-netguard}"
BIN="$OUT/raf-netguard-airgap"
LOGDIR="${RAF_NETGUARD_LOG_DIR:-$OUT/logs}"

mkdir -p "$OUT" "$LOGDIR"

build() {
    CC="${CC:-clang}"
    "$CC" -O2 -Wall -Wextra -Werror -std=c11         "$ROOT/src/native/raf_netguard_airgap.c"         -o "$BIN"
}

ensure_bin() {
    [ -x "$BIN" ] || build
}

hash_logs() {
    prefix="$1"
    for f in "$prefix"*; do
        [ -f "$f" ] || continue
        sha256sum "$f" > "$f.sha256"
        if command -v b3sum >/dev/null 2>&1; then
            b3sum "$f" > "$f.blake3"
        fi
    done
}

audit_exec() {
    [ "$#" -gt 0 ] || { echo "usage: $0 audit command [args...]" >&2; exit 64; }
    command -v strace >/dev/null 2>&1 || {
        echo "RAF_NETGUARD TOKEN_VAZIO: strace unavailable" >&2
        exit 69
    }
    ts=$(date -u +%Y%m%dT%H%M%SZ)
    prefix="$LOGDIR/netguard-$ts"
    set +e
    strace -ff -ttt -s 32         -e trace=socket,connect,bind,listen,accept,accept4,getsockname,getpeername         -o "$prefix" -- "$@"
    rc=$?
    set -e
    hash_logs "$prefix"
    printf '%s\n' "RAF_NETGUARD_AUDIT receipt=$prefix rc=$rc nat=TOKEN_VAZIO pat=TOKEN_VAZIO payload=NOT_CAPTURED"
    return "$rc"
}

case "${1:-}" in
    build)
        build
        ;;
    selftest)
        ensure_bin
        "$BIN" --selftest
        ;;
    airgap)
        shift
        [ "$#" -gt 0 ] || { echo "usage: $0 airgap command [args...]" >&2; exit 64; }
        ensure_bin
        exec "$BIN" "$@"
        ;;
    audit)
        shift
        audit_exec "$@"
        ;;
    audit-airgap)
        shift
        [ "$#" -gt 0 ] || { echo "usage: $0 audit-airgap command [args...]" >&2; exit 64; }
        ensure_bin
        audit_exec "$BIN" "$@"
        ;;
    *)
        cat >&2 <<EOF
usage:
  $0 build
  $0 selftest
  $0 audit command [args...]
  $0 airgap command [args...]
  $0 audit-airgap command [args...]

audit          = metadata-only connection syscall trace for the launched process tree
airgap         = fail-closed seccomp deny of network syscalls, no root
audit-airgap  = records attempted connection syscalls while the airgap blocks them
EOF
        exit 64
        ;;
esac
