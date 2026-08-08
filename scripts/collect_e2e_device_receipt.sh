#!/usr/bin/env bash
set -euo pipefail

# RAFCODEPHI physical-device receipt collector.
# Runs on the installed com.termux.rafacodephi app without root.
# It never promotes a claim; reproduction is intentionally TOKEN_VAZIO here.

PACKAGE="com.termux.rafacodephi"
EXPECTED_PREFIX="/data/data/${PACKAGE}/files/usr"
REPO="rafaelmeloreisnovo/termux-app-rafacodephi"
OUT=""
GIT_COMMIT=""
APK_SHA256=""
BOOTSTRAP_SHA256=""
WORKLOAD="printf 'RAFCODEPHI-E2E-V1\n' | sha256sum"

usage() {
  cat <<'EOF'
Usage:
  bash scripts/collect_e2e_device_receipt.sh \
    --git-commit <40hex> \
    --apk-sha256 <64hex> \
    --bootstrap-sha256 <64hex> \
    [--workload '<shell command>'] \
    [--out reports/device-e2e/<name>.json]

The script is fail-closed:
- installed APK hash must be readable and equal --apk-sha256;
- canonical RAFCODEPHI prefix and required bootstrap commands must exist;
- apt/pkg must be present;
- workload must exit 0.
A single run never sets claim_allowed=true. Use validate_e2e_receipt.py with
--reference against a second independent receipt to evaluate reproduction.
EOF
}

die() {
  printf 'ERROR: %s\n' "$*" >&2
  exit 2
}

is_hex() {
  local value="$1" size="$2"
  [[ ${#value} -eq "$size" && "$value" =~ ^[0-9a-f]+$ ]]
}

while (($#)); do
  case "$1" in
    --git-commit) GIT_COMMIT="${2:-}"; shift 2 ;;
    --apk-sha256) APK_SHA256="${2:-}"; shift 2 ;;
    --bootstrap-sha256) BOOTSTRAP_SHA256="${2:-}"; shift 2 ;;
    --workload) WORKLOAD="${2:-}"; shift 2 ;;
    --out) OUT="${2:-}"; shift 2 ;;
    -h|--help) usage; exit 0 ;;
    *) die "unknown argument: $1" ;;
  esac
done

command -v python3 >/dev/null 2>&1 || die "python3 is required to emit canonical JSON"
command -v sha256sum >/dev/null 2>&1 || die "sha256sum is required"
command -v getprop >/dev/null 2>&1 || die "Android getprop unavailable"
command -v pm >/dev/null 2>&1 || die "Android package manager CLI unavailable"

is_hex "$GIT_COMMIT" 40 || die "--git-commit must be lowercase 40-hex"
is_hex "$APK_SHA256" 64 || die "--apk-sha256 must be lowercase 64-hex"
is_hex "$BOOTSTRAP_SHA256" 64 || die "--bootstrap-sha256 must be lowercase 64-hex"
[[ -n "$WORKLOAD" ]] || die "--workload cannot be empty"

generated_at="$(date --iso-8601=seconds 2>/dev/null || date '+%Y-%m-%dT%H:%M:%S%z')"
manufacturer="$(getprop ro.product.manufacturer)"
model="$(getprop ro.product.model)"
android_release="$(getprop ro.build.version.release)"
abi="$(getprop ro.product.cpu.abi)"

case "$abi" in
  armeabi-v7a|arm64-v8a) ;;
  *) die "unsupported ABI for canonical RAFCODEPHI proof: $abi" ;;
esac

[[ "${PREFIX:-}" == "$EXPECTED_PREFIX" ]] || die "PREFIX drift: ${PREFIX:-<unset>} != $EXPECTED_PREFIX"

required=(
  "$EXPECTED_PREFIX/bin/sh"
  "$EXPECTED_PREFIX/bin/bash"
  "$EXPECTED_PREFIX/bin/pkg"
  "$EXPECTED_PREFIX/bin/apt"
  "$EXPECTED_PREFIX/bin/dpkg"
  "$EXPECTED_PREFIX/bin/proot"
)
for path in "${required[@]}"; do
  [[ -e "$path" ]] || die "required bootstrap path missing: $path"
done

command -v pkg >/dev/null 2>&1 || die "pkg not in PATH"
command -v apt >/dev/null 2>&1 || die "apt not in PATH"
apt --version >/dev/null 2>&1 || die "apt --version failed"

apk_listing="$(pm path "$PACKAGE" 2>/dev/null || true)"
apk_path="$(printf '%s\n' "$apk_listing" | sed -n 's/^package://p' | head -n 1)"
[[ -n "$apk_path" ]] || die "pm path could not resolve installed $PACKAGE"
[[ -r "$apk_path" ]] || die "installed APK is not readable: $apk_path"
installed_apk_sha256="$(sha256sum "$apk_path" | awk '{print $1}')"
[[ "$installed_apk_sha256" == "$APK_SHA256" ]] || \
  die "installed APK hash mismatch: expected=$APK_SHA256 observed=$installed_apk_sha256"

workdir="$(mktemp -d)"
trap 'rm -rf "$workdir"' EXIT
stdout_file="$workdir/workload.stdout"
stderr_file="$workdir/workload.stderr"

set +e
bash -lc "$WORKLOAD" >"$stdout_file" 2>"$stderr_file"
workload_rc=$?
set -e
[[ "$workload_rc" -eq 0 ]] || {
  printf '%s\n' '--- workload stderr ---' >&2
  cat "$stderr_file" >&2 || true
  die "workload failed with exit code $workload_rc"
}
stdout_sha256="$(sha256sum "$stdout_file" | awk '{print $1}')"

tmp_json="$workdir/receipt.json"

python3 - "$tmp_json" <<PY
import hashlib, json, pathlib
out = pathlib.Path(${tmp_json@Q})
doc = {
    "schema": "rafcodephi.e2e-device-receipt/v1",
    "receipt_id": "0" * 64,
    "generated_at": ${generated_at@Q},
    "provenance": {
        "repository": ${REPO@Q},
        "git_commit": ${GIT_COMMIT@Q},
        "apk_sha256": ${APK_SHA256@Q},
        "bootstrap_sha256": ${BOOTSTRAP_SHA256@Q},
    },
    "device": {
        "manufacturer": ${manufacturer@Q},
        "model": ${model@Q},
        "android_release": ${android_release@Q},
        "abi": ${abi@Q},
        "package": ${PACKAGE@Q},
        "prefix": ${EXPECTED_PREFIX@Q},
        "installed_apk_path": ${apk_path@Q},
        "installed_apk_sha256": ${installed_apk_sha256@Q},
    },
    "workload": {
        "command": ${WORKLOAD@Q},
        "exit_code": int(${workload_rc@Q}),
        "stdout_sha256": ${stdout_sha256@Q},
    },
    "stages": {
        "packages": "PASS",
        "bootstrap": "PASS",
        "apk": "PASS",
        "device": "PASS",
        "workload": "PASS",
        "receipt": "PASS",
        "reproduction": "TOKEN_VAZIO",
    },
    "claim_allowed": False,
}
canonical = json.loads(json.dumps(doc))
canonical["receipt_id"] = "0" * 64
payload = json.dumps(canonical, sort_keys=True, separators=(",", ":"), ensure_ascii=False).encode()
doc["receipt_id"] = hashlib.sha256(payload).hexdigest()
out.write_text(json.dumps(doc, indent=2, sort_keys=True, ensure_ascii=False) + "\n", encoding="utf-8")
PY

if [[ -z "$OUT" ]]; then
  OUT="reports/device-e2e/${generated_at//[:+]/_}-${abi}.json"
fi
mkdir -p "$(dirname "$OUT")"
cp "$tmp_json" "$OUT"

printf 'receipt=%s\n' "$OUT"
printf 'receipt_sha256=%s\n' "$(sha256sum "$OUT" | awk '{print $1}')"
printf 'claim_allowed=false\n'
printf 'reproduction=TOKEN_VAZIO\n'
