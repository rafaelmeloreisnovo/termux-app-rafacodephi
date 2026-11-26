#!/usr/bin/env bash
set -euo pipefail
umask 077  # Device receipts are private unless explicitly reviewed for disclosure.

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

command -v python3 >/dev/null 2>&1 || die "PYTHON_NOT_INSTALLED"
python3 -c 'import json, hashlib' >/dev/null 2>&1 || die "PYTHON_RUNTIME_BROKEN: dynamic linker or Python initialization failed"
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

# /data/user/0 and /data/data may be aliases for this app's private prefix.
[[ -n "${PREFIX:-}" ]] || die "PREFIX_UNSET"
actual_prefix="$(readlink -f "$PREFIX")" || die "PREFIX_UNRESOLVABLE"
expected_prefix="$(readlink -f "$EXPECTED_PREFIX")" || die "EXPECTED_PREFIX_UNRESOLVABLE"
[[ -d "$actual_prefix" && "$actual_prefix" == "$expected_prefix" ]] || die "PREFIX_DRIFT"

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

# Never substitute an inferred APK path or falsely mark runtime custody PASS.
apk_listing="$(pm path "$PACKAGE" 2>"$workdir/pm.stderr")" || \
  die "ANDROID_PACKAGE_MANAGER_UNAVAILABLE: pm path failed; installed APK hash unverified"
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
# Pass values as argv with a quoted heredoc: no shell text may become Python source.
# This remains local diagnostic evidence and cannot self-promote to device E2E.
python3 - \
  "$tmp_json" "$generated_at" "$APP_REPO" "$APP_GIT_COMMIT" \
  "$BUILD_RECEIPT_SHA256" "$APK_SHA256" "$BOOTSTRAP_SOURCE_REPOSITORY" \
  "$BOOTSTRAP_SOURCE_COMMIT" "$BOOTSTRAP_SOURCE_MANIFEST_SHA256" \
  "$BOOTSTRAP_SHA256" "$BOOTSTRAP_PROFILE_SHA256" \
  "$manufacturer" "$model" "$android_release" "$abi" "$PACKAGE" \
  "$EXPECTED_PREFIX" "$apk_path" "$installed_apk_sha256" \
  "$WORKLOAD" "$workload_rc" "$stdout_sha256" <<'PY'
import hashlib
import json
import pathlib
import sys

(
    output, generated_at, app_repo, app_commit, build_receipt_sha256,
    apk_sha256, bootstrap_source_repository, bootstrap_source_commit,
    bootstrap_source_manifest_sha256, bootstrap_sha256, bootstrap_profile_sha256,
    manufacturer, model, android_release, abi, package, prefix,
    apk_path, installed_apk_sha256, workload, workload_rc, stdout_sha256,
) = sys.argv[1:]

out = pathlib.Path(output)
doc = {
    "schema": "rafcodephi.e2e-device-receipt/v1",
    "receipt_id": "0" * 64,
    "generated_at": generated_at,
    "provenance": {
        "repository": app_repo,
        "git_commit": app_commit,
        "build_receipt_sha256": build_receipt_sha256,
        "apk_sha256": apk_sha256,
        "bootstrap_source_repository": bootstrap_source_repository,
        "bootstrap_source_commit": bootstrap_source_commit,
        "bootstrap_source_manifest_sha256": bootstrap_source_manifest_sha256,
        "bootstrap_sha256": bootstrap_sha256,
        "bootstrap_profile_sha256": bootstrap_profile_sha256,
    },
    "device": {
        "manufacturer": manufacturer,
        "model": model,
        "android_release": android_release,
        "abi": abi,
        "package": package,
        "prefix": prefix,
        "installed_apk_path": apk_path,
        "installed_apk_sha256": installed_apk_sha256,
    },
    "workload": {
        "command": workload,
        "exit_code": int(workload_rc),
        "stdout_sha256": stdout_sha256,
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
# Append-only: never overwrite a previous device observation, even on retry.
receipt_dir="$(dirname -- "$OUT")"
mkdir -p -- "$receipt_dir"
receipt_tmp="$(mktemp "$receipt_dir/.e2e-receipt.XXXXXXXX")" || die "RECEIPT_TEMP_CREATE_FAILED"
trap 'rm -rf "$workdir"; rm -f "$receipt_tmp"' EXIT
cp -- "$tmp_json" "$receipt_tmp" || die "RECEIPT_TEMP_WRITE_FAILED"
chmod 600 "$receipt_tmp" || die "RECEIPT_PERMISSIONS_FAILED"
# Hard link is atomic and fails if the destination already exists (also blocks symlinks).
ln -- "$receipt_tmp" "$OUT" || die "RECEIPT_DESTINATION_EXISTS_OR_LINK_FAILED"
rm -f -- "$receipt_tmp"
receipt_tmp=""


printf 'receipt=%s\n' "$OUT"
printf 'receipt_sha256=%s\n' "$(sha256sum "$OUT" | awk '{print $1}')"
printf 'build_receipt_sha256=%s\n' "$BUILD_RECEIPT_SHA256"
printf 'claim_allowed=false\n'
printf 'reproduction=TOKEN_VAZIO\n'
