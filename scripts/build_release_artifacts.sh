#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
DIST_DIR="$ROOT_DIR/dist/release-artifacts"
cd "$ROOT_DIR"

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

if [[ "${TERMUX_ENABLE_RELEASE_SIGNING:-false}" == "true" ]]; then
  signed_count=$(find "$DIST_DIR" -type f -name '*release*.apk' ! -name '*unsigned*' | wc -l | tr -d ' ')
  [[ "$signed_count" -gt 0 ]] || { echo "❌ Signing enabled but no signed release APK found" >&2; exit 3; }
  signing_status="signed"
else
  unsigned_count=$(find "$DIST_DIR" -type f -name '*release*unsigned*.apk' | wc -l | tr -d ' ')
  [[ "$unsigned_count" -gt 0 ]] || { echo "❌ Signing disabled but unsigned release APK not found" >&2; exit 4; }
  signing_status="unsigned"
fi

( cd "$DIST_DIR" && sha256sum *.apk > SHA256SUMS.txt )

echo "== Release Artifact Summary =="
echo "debug_apks=${debug_count}"
echo "release_apks=${release_count}"
echo "abis=armeabi-v7a,arm64-v8a,x86_64"
echo "universal_apks=${universal_count}"
echo "signing_status=${signing_status}"
cat "$DIST_DIR/SHA256SUMS.txt"
