#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
DIST_DIR="$ROOT_DIR/dist/release-artifacts"
UNSIGNED_DIR="$DIST_DIR/unsigned"
SIGNED_DIR="$DIST_DIR/signed"
cd "$ROOT_DIR"

if [[ "${TERMUX_BOOTSTRAP_VALIDATION_MODE:-}" == "upstream-debug-compat" ]]; then
  echo "❌ TERMUX_BOOTSTRAP_VALIDATION_MODE=upstream-debug-compat is blocked for release artifact lanes" >&2
  exit 9
fi

./scripts/ensure_android_sdk.sh "$ROOT_DIR"
eval "$(./scripts/prepare_bootstrap_env.sh --print-env)"

export TERMUX_SPLIT_APKS_FOR_DEBUG_BUILDS=1
export TERMUX_SPLIT_APKS_FOR_RELEASE_BUILDS=1

IFS="," read -r -a required_abis <<< "${TERMUX_REQUIRED_ABIS:-armeabi-v7a,arm64-v8a}"

resolve_sdk_dir() {
  local sdk_dir=""
  if [[ -f local.properties ]]; then
    sdk_dir="$(awk -F= '/^sdk.dir=/{print $2; exit}' local.properties)"
    sdk_dir="${sdk_dir//\\/}"
  fi
  if [[ -z "$sdk_dir" && -n "${ANDROID_HOME:-}" ]]; then
    sdk_dir="$ANDROID_HOME"
  fi
  if [[ -z "$sdk_dir" && -n "${ANDROID_SDK_ROOT:-}" ]]; then
    sdk_dir="$ANDROID_SDK_ROOT"
  fi
  if [[ -z "$sdk_dir" ]]; then
    echo "❌ Unable to resolve Android SDK directory from local.properties/ANDROID_HOME/ANDROID_SDK_ROOT" >&2
    exit 5
  fi
  printf '%s\n' "$sdk_dir"
}

collect_apks() {
  local target_dir="$1"
  rm -rf "$target_dir"
  mkdir -p "$target_dir"
  find app/build/outputs/apk -type f -name '*.apk' -print0 | xargs -0 -I{} cp {} "$target_dir/"
}

validate_required_abis() {
  local target_dir="$1"
  for abi in "${required_abis[@]}"; do
    local count
    count=$(find "$target_dir" -type f -name "*${abi}*.apk" | wc -l | tr -d ' ')
    if [[ "$count" -eq 0 ]]; then
      echo "❌ Missing required ABI APK in ${target_dir}: ${abi}" >&2
      exit 2
    fi
  done
}

run_unsigned_lane() {
  echo "== Building unsigned lane =="
  unset TERMUX_ENABLE_RELEASE_SIGNING TERMUX_RELEASE_KEYSTORE_FILE TERMUX_RELEASE_KEYSTORE_PASSWORD TERMUX_RELEASE_KEY_ALIAS TERMUX_RELEASE_KEY_PASSWORD
  ./gradlew :app:clean :app:assembleDebug :app:assembleRelease --no-daemon "$@"
  collect_apks "$UNSIGNED_DIR"
  validate_required_abis "$UNSIGNED_DIR"
}

run_signed_lane() {
  if [[ "${TERMUX_ENABLE_RELEASE_SIGNING:-false}" != "true" ]]; then
    echo "⚠️ Skipping signed lane (TERMUX_ENABLE_RELEASE_SIGNING!=true)."
    return
  fi
  echo "== Building signed lane =="
  ./gradlew :app:clean :app:assembleRelease --no-daemon "$@"
  collect_apks "$SIGNED_DIR"
  validate_required_abis "$SIGNED_DIR"
}

run_unsigned_lane "$@"
run_signed_lane "$@"

SDK_DIR="$(resolve_sdk_dir)"
APKSIGNER="$SDK_DIR/build-tools/35.0.0/apksigner"
if [[ ! -x "$APKSIGNER" ]]; then
  echo "❌ apksigner not found at $APKSIGNER" >&2
  exit 5
fi

unsigned_release_count=$(find "$UNSIGNED_DIR" -type f -name '*release*.apk' | wc -l | tr -d ' ')
[[ "$unsigned_release_count" -gt 0 ]] || { echo "❌ Unsigned lane did not generate release APKs" >&2; exit 4; }

unsigned_verified=0
while IFS= read -r -d '' apk; do
  if ! "$APKSIGNER" verify "$apk" >/dev/null 2>&1; then
    unsigned_verified=$((unsigned_verified+1))
  fi
done < <(find "$UNSIGNED_DIR" -type f -name '*release*.apk' -print0)
[[ "$unsigned_verified" -gt 0 ]] || { echo "❌ Unsigned lane release APKs appear signed unexpectedly" >&2; exit 4; }

signed_status="not-generated"
if [[ -d "$SIGNED_DIR" ]]; then
  signed_release_count=$(find "$SIGNED_DIR" -type f -name '*release*.apk' | wc -l | tr -d ' ')
  if [[ "$signed_release_count" -gt 0 ]]; then
    signed_verified=0
    while IFS= read -r -d '' apk; do
      if "$APKSIGNER" verify "$apk" >/dev/null 2>&1; then
        signed_verified=$((signed_verified+1))
      fi
    done < <(find "$SIGNED_DIR" -type f -name '*release*.apk' -print0)
    [[ "$signed_verified" -gt 0 ]] || { echo "❌ Signed lane enabled but no signed release APK found" >&2; exit 3; }
    signed_status="generated"
  fi
fi

mkdir -p "$DIST_DIR"
( cd "$DIST_DIR" && find unsigned signed -type f -name '*.apk' -print0 2>/dev/null | xargs -0 sha256sum > SHA256SUMS.txt )
( cd "$DIST_DIR" && {
  echo "artifact_dir=$DIST_DIR"
  echo "generated_at_utc=$(date -u +%Y-%m-%dT%H:%M:%SZ)"
  echo "required_abis=${required_abis[*]}"
  echo "unsigned_dir=$UNSIGNED_DIR"
  echo "signed_dir=$SIGNED_DIR"
  echo "signed_status=$signed_status"
  find unsigned signed -type f -name '*.apk' -printf "%P\n" 2>/dev/null | sort
} > ARTIFACT_MANIFEST.txt )

echo "== Release Artifact Summary =="
echo "required_abis=${required_abis[*]}"
echo "unsigned_dir=$UNSIGNED_DIR"
echo "signed_dir=$SIGNED_DIR"
echo "signed_status=$signed_status"
cat "$DIST_DIR/ARTIFACT_MANIFEST.txt"
cat "$DIST_DIR/SHA256SUMS.txt"
