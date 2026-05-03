#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
DIST_DIR="$ROOT_DIR/dist/release-artifacts"
cd "$ROOT_DIR"

if [[ "${TERMUX_BOOTSTRAP_VALIDATION_MODE:-}" == "upstream-debug-compat" ]]; then
  echo "❌ TERMUX_BOOTSTRAP_VALIDATION_MODE=upstream-debug-compat is blocked for release artifact lanes" >&2
  exit 9
fi

./scripts/ensure_android_sdk.sh "$ROOT_DIR"
eval "$(./scripts/prepare_bootstrap_env.sh --print-env)"

export TERMUX_SPLIT_APKS_FOR_DEBUG_BUILDS=1
export TERMUX_SPLIT_APKS_FOR_RELEASE_BUILDS=1

./gradlew :app:assembleDebug :app:assembleRelease --no-daemon "$@"

mkdir -p "$DIST_DIR"
find app/build/outputs/apk -type f -name '*.apk' -print0 | xargs -0 -I{} cp {} "$DIST_DIR/"

debug_count=$(find "$DIST_DIR" -type f -name '*debug*.apk' | wc -l | tr -d ' ')
release_count=$(find "$DIST_DIR" -type f -name '*release*.apk' | wc -l | tr -d ' ')

for abi in armeabi-v7a arm64-v8a x86_64; do
  count=$(find "$DIST_DIR" -type f -name "*${abi}*.apk" | wc -l | tr -d ' ')
  if [[ "$count" -eq 0 ]]; then
    echo "❌ Missing required ABI APK: ${abi}" >&2
    exit 2
  fi
done

universal_count=$(find "$DIST_DIR" -type f -name '*universal*.apk' | wc -l | tr -d ' ')

SDK_DIR="$(grep -E '^sdk.dir=' local.properties | cut -d= -f2-)"
SDK_DIR="${SDK_DIR//\\/}"
APKSIGNER="$SDK_DIR/build-tools/35.0.0/apksigner"

if [[ ! -x "$APKSIGNER" ]]; then
  echo "❌ apksigner not found at $APKSIGNER" >&2
  exit 5
fi

if [[ "${TERMUX_ENABLE_RELEASE_SIGNING:-false}" == "true" ]]; then
  signed_count=0
  while IFS= read -r -d '' apk; do
    if "$APKSIGNER" verify "$apk" >/dev/null 2>&1; then
      signed_count=$((signed_count + 1))
    fi
  done < <(find "$DIST_DIR" -type f -name '*release*.apk' -print0)
  [[ "$signed_count" -gt 0 ]] || { echo "❌ Signing enabled but no signed release APK found" >&2; exit 3; }
  signing_status="signed"
else
  unsigned_count=0
  while IFS= read -r -d '' apk; do
    if ! "$APKSIGNER" verify "$apk" >/dev/null 2>&1; then
      unsigned_count=$((unsigned_count + 1))
    fi
  done < <(find "$DIST_DIR" -type f -name '*release*.apk' -print0)
  [[ "$unsigned_count" -gt 0 ]] || { echo "❌ Signing disabled but unsigned release APK not found" >&2; exit 4; }
  signing_status="unsigned"
fi

( cd "$DIST_DIR" && sha256sum *.apk > SHA256SUMS.txt )
( cd "$DIST_DIR" && {
  echo "artifact_dir=$DIST_DIR";
  echo "generated_at_utc=$(date -u +%Y-%m-%dT%H:%M:%SZ)";
  echo "signing_status=$signing_status";
  echo "required_abis=armeabi-v7a,arm64-v8a,x86_64";
  find . -maxdepth 1 -type f -name "*.apk" -printf "%f\n" | sort;
} > ARTIFACT_MANIFEST.txt )

echo "== Release Artifact Summary =="
echo "debug_apks=${debug_count}"
echo "release_apks=${release_count}"
echo "abis=armeabi-v7a,arm64-v8a,x86_64"
echo "universal_apks=${universal_count}"
echo "signing_status=${signing_status}"
cat "$DIST_DIR/ARTIFACT_MANIFEST.txt"
cat "$DIST_DIR/SHA256SUMS.txt"
