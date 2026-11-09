#!/usr/bin/env bash
# Hosted build-time handoff only. No APK, bootstrap or runtime mutation.
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
fail() { printf '[apk-source-contract] ERROR: %s\n' "$*" >&2; exit 1; }

[[ "$#" -eq 1 ]] || fail "usage: append_apk_source_contract.sh ARTIFACT_MANIFEST.txt"
artifact_manifest="$1"
[[ -s "$artifact_manifest" ]] || fail "artifact manifest missing or empty: $artifact_manifest"
grep -q '^artifact_dir=' "$artifact_manifest" || fail "artifact manifest header missing"
if grep -q '^TERMUX_PACKAGES_RAF_REPO=' "$artifact_manifest"; then
  fail "source contract already embedded; refuse duplicate receipt"
fi

source_dir="${TERMUX_PACKAGES_RAF_MANIFEST_DIR:-dist/source-contract}"
if [[ "$source_dir" != /* ]]; then
  source_dir="$ROOT_DIR/$source_dir"
fi
source_manifest="$source_dir/TERMUX_PACKAGES_RAFCODEPHI_SOURCE.env"
[[ -s "$source_manifest" ]] || fail "source contract missing or empty: $source_manifest"
grep -Eq '^TERMUX_PACKAGES_RAF_RESOLVED_COMMIT=[0-9a-f]{40}$' "$source_manifest" ||
  fail "resolved producer commit not a full lowercase SHA"
grep -Fxq 'TERMUX_PACKAGES_RAF_ABIS=armeabi-v7a,arm64-v8a' "$source_manifest" ||
  fail "producer ABI pair missing or invalid"
grep -Fxq 'TERMUX_PACKAGES_RAF_ROLE=source-built-packages-bootstrap-and-apt' "$source_manifest" ||
  fail "producer role missing or invalid"

# All preconditions checked before writing. The parent regenerates the manifest
# per build; the duplicate guard prevents accidental standalone replay.
cat "$source_manifest" >> "$artifact_manifest"
