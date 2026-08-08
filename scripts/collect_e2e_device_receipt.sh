#!/usr/bin/env bash
set -euo pipefail

# RAFCODEPHI physical-device receipt collector.
# Runs inside installed com.termux.rafacodephi without root.
# It consumes a build receipt that already binds termux-packages bootstrap → APK.
# A single run never self-promotes reproducibility.

PACKAGE="com.termux.rafacodephi"
EXPECTED_PREFIX="/data/data/${PACKAGE}/files/usr"
APP_REPO="rafaelmeloreisnovo/termux-app-rafacodephi"
SOURCE_REPO="rafaelmeloreisnovo/termux-packages"
OUT=""
BUILD_RECEIPT=""
WORKLOAD="printf 'RAFCODEPHI-E2E-V1\n' | sha256sum"

usage() {
  cat <<'EOF'
Usage:
  bash scripts/collect_e2e_device_receipt.sh \
    --build-receipt <e2e-build-receipt.json> \
    [--workload '<shell command>'] \
    [--out reports/device-e2e/<name>.json]

The build receipt must have been emitted by scripts/emit_e2e_build_receipt.py.
The collector verifies:
- canonical RAFCODEPHI package/prefix/ABI;
- required bootstrap shell/pkg/apt/dpkg/proot surface;
- installed base.apk SHA-256 equals the build receipt APK SHA-256;
- build receipt binds a real-pkg termux-packages source manifest;
- workload exits 0 and stdout is hashed.

A single device receipt records reproduction=TOKEN_VAZIO and claim_allowed=false.
EOF
}

die() {
  printf 'ERROR: %s\n' "$*" >&2
  exit 2
}

while (($#)); do
  case "$1" in
    --build-receipt) BUILD_RECEIPT="${2:-}"; shift 2 ;;
    --workload) WORKLOAD="${2:-}"; shift 2 ;;
    --out) OUT="${2:-}"; shift 2 ;;
    -h|--help) usage; exit 0 ;;
    *) die "unknown argument: $1" ;;
  esac
done

command -v python3 >/dev/null 2>&1 || die "python3 is required"
command -v sha256sum >/dev/null 2>&1 || die "sha256sum is required"
command -v getprop >/dev/null 2>&1 || die "Android getprop unavailable"
command -v pm >/dev/null 2>&1 || die "Android package manager CLI unavailable"

[[ -n "$BUILD_RECEIPT" && -r "$BUILD_RECEIPT" ]] || die "--build-receipt must name a readable file"
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

workdir="$(mktemp -d)"
trap 'rm -rf "$workdir"' EXIT
meta_file="$workdir/build-meta.txt"

if ! python3 - "$BUILD_RECEIPT" "$abi" >"$meta_file" <<'PY'
import json, re, sys
from pathlib import Path

path = Path(sys.argv[1])
abi = sys.argv[2]
doc = json.loads(path.read_text(encoding="utf-8"))
if not isinstance(doc, dict):
    raise SystemExit("build receipt must be an object")

expected = {
    "schema": "rafcodephi.e2e-build-receipt/v1",
    "repository": "rafaelmeloreisnovo/termux-app-rafacodephi",
    "android_abi": abi,
    "build_gate": "PASS",
    "device_runtime": "TOKEN_VAZIO",
    "claim_allowed": False,
}
drift = [k for k, v in expected.items() if doc.get(k) != v]
if drift:
    raise SystemExit("build receipt drift: " + ", ".join(drift))

hex40 = re.compile(r"^[0-9a-f]{40}$")
hex64 = re.compile(r"^[0-9a-f]{64}$")
if not hex40.fullmatch(str(doc.get("git_commit", ""))):
    raise SystemExit("invalid app git_commit")
if not hex64.fullmatch(str(doc.get("apk_sha256", ""))):
    raise SystemExit("invalid apk_sha256")

bootstrap = doc.get("bootstrap")
if not isinstance(bootstrap, dict):
    raise SystemExit("bootstrap block missing")
if bootstrap.get("source_repository") != "rafaelmeloreisnovo/termux-packages":
    raise SystemExit("bootstrap source repository drift")
if not hex40.fullmatch(str(bootstrap.get("source_git_commit", ""))):
    raise SystemExit("invalid source_git_commit")
for key in ("source_manifest_sha256", "artifact_sha256", "profile_sha256"):
    if not hex64.fullmatch(str(bootstrap.get(key, ""))):
        raise SystemExit(f"invalid bootstrap.{key}")

print(doc["git_commit"])
print(doc["apk_sha256"])
print(bootstrap["source_repository"])
print(bootstrap["source_git_commit"])
print(bootstrap["source_manifest_sha256"])
print(bootstrap["artifact_sha256"])
print(bootstrap["profile_sha256"])
PY
then
  die "build receipt validation failed"
fi

mapfile -t meta <"$meta_file"
[[ "${#meta[@]}" -eq 7 ]] || die "build receipt metadata arity mismatch"
APP_GIT_COMMIT="${meta[0]}"
APK_SHA256="${meta[1]}"
BOOTSTRAP_SOURCE_REPOSITORY="${meta[2]}"
BOOTSTRAP_SOURCE_COMMIT="${meta[3]}"
BOOTSTRAP_SOURCE_MANIFEST_SHA256="${meta[4]}"
BOOTSTRAP_SHA256="${meta[5]}"
BOOTSTRAP_PROFILE_SHA256="${meta[6]}"
BUILD_RECEIPT_SHA256="$(sha256sum "$BUILD_RECEIPT" | awk '{print $1}')"

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
        "repository": ${APP_REPO@Q},
        "git_commit": ${APP_GIT_COMMIT@Q},
        "build_receipt_sha256": ${BUILD_RECEIPT_SHA256@Q},
        "apk_sha256": ${APK_SHA256@Q},
        "bootstrap_source_repository": ${BOOTSTRAP_SOURCE_REPOSITORY@Q},
        "bootstrap_source_commit": ${BOOTSTRAP_SOURCE_COMMIT@Q},
        "bootstrap_source_manifest_sha256": ${BOOTSTRAP_SOURCE_MANIFEST_SHA256@Q},
        "bootstrap_sha256": ${BOOTSTRAP_SHA256@Q},
        "bootstrap_profile_sha256": ${BOOTSTRAP_PROFILE_SHA256@Q},
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
printf 'build_receipt_sha256=%s\n' "$BUILD_RECEIPT_SHA256"
printf 'claim_allowed=false\n'
printf 'reproduction=TOKEN_VAZIO\n'
