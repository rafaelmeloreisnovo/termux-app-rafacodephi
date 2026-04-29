#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT_DIR"

./scripts/ensure_android_sdk.sh "$ROOT_DIR"

export TERMUX_SPLIT_APKS_FOR_DEBUG_BUILDS="${TERMUX_SPLIT_APKS_FOR_DEBUG_BUILDS:-1}"
export TERMUX_SPLIT_APKS_FOR_RELEASE_BUILDS="${TERMUX_SPLIT_APKS_FOR_RELEASE_BUILDS:-1}"

./gradlew :app:assembleDebug :app:assembleRelease --no-daemon "$@"

echo "\n== Artifact check =="
arm32_count=$(find app/build/outputs/apk -type f -name '*armeabi-v7a*.apk' | wc -l | tr -d ' ')
arm64_count=$(find app/build/outputs/apk -type f -name '*arm64-v8a*.apk' | wc -l | tr -d ' ')
unsigned_count=$(find app/build/outputs/apk/release -type f -name '*unsigned*.apk' | wc -l | tr -d ' ')

printf 'armeabi-v7a APKs: %s\n' "$arm32_count"
printf 'arm64-v8a APKs: %s\n' "$arm64_count"
printf 'unsigned release APKs: %s\n' "$unsigned_count"

if [[ "$arm32_count" -eq 0 || "$arm64_count" -eq 0 ]]; then
  echo "❌ Missing required arm32/arm64 artifacts"
  exit 2
fi

if [[ "${TERMUX_ENABLE_RELEASE_SIGNING:-false}" == "true" ]]; then
  signed_count=$(find app/build/outputs/apk/release -type f -name '*.apk' ! -name '*unsigned*' | wc -l | tr -d ' ')
  printf 'signed release APKs: %s\n' "$signed_count"
  if [[ "$signed_count" -eq 0 ]]; then
    echo "❌ Signing requested but no signed APK detected"
    exit 3
  fi
fi

echo "✅ Artifact matrix generated successfully"
