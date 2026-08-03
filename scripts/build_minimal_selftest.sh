#!/usr/bin/env sh
# RAFCODEΦ minimal self-test build: no Gradle, Java, Python, Make, Git or network.
set -eu

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
SRC="$ROOT/bootstrap_rafaelia"
OUT="${RAF_MINIMAL_OUT:-$ROOT/build/minimal}"
BIN="$OUT/raf_selftest"
LOG="$OUT/selftest.log"
RECEIPT="$OUT/receipt.env"

fail() { printf '%s\n' "[raf-minimal] ERROR: $*" >&2; exit 1; }
command -v mkdir >/dev/null 2>&1 || fail "mkdir not found"
command -v uname >/dev/null 2>&1 || fail "uname not found"

CC_BIN=${CC:-}
if [ -z "$CC_BIN" ]; then
  if command -v clang >/dev/null 2>&1; then CC_BIN=clang
  elif command -v cc >/dev/null 2>&1; then CC_BIN=cc
  else fail "C compiler not found (set CC or install clang)"
  fi
fi
command -v "$CC_BIN" >/dev/null 2>&1 || fail "compiler not executable: $CC_BIN"

[ -f "$SRC/raf_selftest.c" ] || fail "missing $SRC/raf_selftest.c"
[ -f "$SRC/raf_main.c" ] || fail "missing $SRC/raf_main.c"
mkdir -p "$OUT"

ARCH=$(uname -m 2>/dev/null || printf unknown)
CFLAGS=${CFLAGS:--O2 -fno-strict-aliasing -Wall -Wextra -Werror=implicit-function-declaration}
printf '%s\n' "[raf-minimal] compiler=$CC_BIN arch=$ARCH out=$OUT"
"$CC_BIN" $CFLAGS -I"$SRC" "$SRC/raf_selftest.c" "$SRC/raf_main.c" -o "$BIN"

OUTPUT=$($BIN 2>&1) || { STATUS=$?; printf '%s\n' "$OUTPUT" > "$LOG"; fail "self-test exited $STATUS"; }
printf '%s\n' "$OUTPUT" | tee "$LOG"
case "$OUTPUT" in
  *"ok="*" fail=0"*|*"SELFTEST total_fail 0"*) : ;;
  *) fail "self-test did not report zero failures" ;;
esac

BIN_SHA=TOKEN_VAZIO_SHA256_TOOL_UNAVAILABLE
if command -v sha256sum >/dev/null 2>&1; then
  BIN_SHA=$(sha256sum "$BIN" | { read -r h rest; printf '%s' "$h"; })
elif command -v shasum >/dev/null 2>&1; then
  BIN_SHA=$(shasum -a 256 "$BIN" | { read -r h rest; printf '%s' "$h"; })
fi

{
  printf 'schema_version=1\n'
  printf 'state=PASS_LOCAL_LIMITED\n'
  printf 'claim_allowed=false\n'
  printf 'compiler=%s\n' "$CC_BIN"
  printf 'architecture=%s\n' "$ARCH"
  printf 'binary=%s\n' "$BIN"
  printf 'binary_sha256=%s\n' "$BIN_SHA"
  printf 'test_summary=%s\n' "$(printf '%s' "$OUTPUT" | tr '\n' ' ')"
} > "$RECEIPT"

printf '%s\n' "[raf-minimal] PASS receipt=$RECEIPT"
