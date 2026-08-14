#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
OUT_DIR="${ROOT_DIR}/dist/apk-matrix"
UNSIGNED_DIR="${OUT_DIR}/unsigned"
SIGNED_DIR="${OUT_DIR}/signed"
DEFAULT_KEYSTORE_PATH="${ROOT_DIR}/dist/local-release.keystore"
KEYSTORE_PATH="${KEYSTORE_PATH:-${DEFAULT_KEYSTORE_PATH}}"
KEY_ALIAS="${KEY_ALIAS:-localrelease}"
KEY_PASS="${KEY_PASS:-changeit}"
STORE_PASS="${STORE_PASS:-changeit}"
RELEASE_TRACK="${RELEASE_TRACK:-internal}"
APK_MATRIX_UNIT_TESTS_REQUIRED="${APK_MATRIX_UNIT_TESTS_REQUIRED:-true}"

info() { printf '\n[build_apk_matrix] %s\n' "$*"; }
fail() { printf '\n[build_apk_matrix] ERROR: %s\n' "$*" >&2; exit 1; }

detect_apk_abi() {
  local base="$1"
  case "${base}" in
    *arm64-v8a*) printf 'arm64-v8a' ;;
    *armeabi-v7a*) printf 'armeabi-v7a' ;;
    *x86_64*) printf 'x86_64' ;;
    *x86*) printf 'x86' ;;
    *universal*) printf 'universal' ;;
    *) printf 'unknown' ;;
  esac
}

record_diag() {
  mkdir -p "${OUT_DIR}"
  printf '%s\n' "$*" >> "${OUT_DIR}/APK_MATRIX_DIAGNOSTIC.md"
}

cd "${ROOT_DIR}"
source "${ROOT_DIR}/scripts/abi_policy_lib.sh"
required_abis=( $(abi_policy_required_array) )

info "Applying Android boot hotfixes"
python3 "${ROOT_DIR}/scripts/apply_android_boot_hotfixes.py"

info "Provisioning Android SDK/NDK/CMake"
./scripts/setup_android_toolchain.sh

case "${RELEASE_TRACK}" in
  official|internal) ;;
  *) fail "RELEASE_TRACK must be official or internal, got ${RELEASE_TRACK}" ;;
esac

if [[ "${RELEASE_TRACK}" == "official" && "${KEYSTORE_PATH}" == "${DEFAULT_KEYSTORE_PATH}" ]]; then
  fail "Official track requires an explicit release keystore; local validation signing is internal-only."
fi

mkdir -p "${UNSIGNED_DIR}" "${SIGNED_DIR}" "$(dirname "${KEYSTORE_PATH}")"
printf '# APK Matrix Diagnostic\n\n' > "${OUT_DIR}/APK_MATRIX_DIAGNOSTIC.md"
record_diag "- release_track: ${RELEASE_TRACK}"
record_diag "- required_abis: ${required_abis[*]}"
record_diag "- unit_tests_required: ${APK_MATRIX_UNIT_TESTS_REQUIRED}"

info "Validating RAFCODEPHI packages source contract"
./scripts/validate_rafcodephi_packages_source.sh
record_diag "- packages_source_contract: success"

info "Preparing bootstrap environment and BLAKE3 vars"
# The APK matrix accepts the three bootstrap payload routes implemented by
# prepare_bootstrap_env.sh. source-built-real is intentionally first-class here:
# beta/release callers may pin a source-built ARM + AArch64 pair and must not be
# silently normalized back to the local bridge payload.
BOOTSTRAP_SOURCE_REQUESTED="${RAF_BOOTSTRAP_SOURCE:-local}"
case "${BOOTSTRAP_SOURCE_REQUESTED}" in
  local|upstream|source-built-real)
    ;;
  termux-packagesRafcodephi-source-contract)
    info "RAF_BOOTSTRAP_SOURCE=${BOOTSTRAP_SOURCE_REQUESTED} is a package-source contract marker; using local bootstrap payload generation"
    BOOTSTRAP_SOURCE_REQUESTED="local"
    ;;
  *)
    fail "RAF_BOOTSTRAP_SOURCE must be local, upstream, or source-built-real, got ${BOOTSTRAP_SOURCE_REQUESTED}"
    ;;
esac
export RAF_BOOTSTRAP_SOURCE="${BOOTSTRAP_SOURCE_REQUESTED}"
record_diag "- bootstrap_source: ${RAF_BOOTSTRAP_SOURCE}"
info "Using RAF_BOOTSTRAP_SOURCE=${RAF_BOOTSTRAP_SOURCE}"
bootstrap_env_output="$(./scripts/prepare_bootstrap_env.sh --print-env)" || fail "Bootstrap environment setup failed"
eval "${bootstrap_env_output}" || fail "Failed to evaluate bootstrap environment"
record_diag "- bootstrap_env: success"

bootstrap_hash_keys=()
for abi in "${required_abis[@]}"; do
  case "${abi}" in
    arm64-v8a) bootstrap_hash_keys+=(AARCH64) ;;
    armeabi-v7a) bootstrap_hash_keys+=(ARM) ;;
    x86_64) bootstrap_hash_keys+=(X86_64) ;;
    x86) bootstrap_hash_keys+=(I686) ;;
    *) fail "Unsupported ABI in policy: ${abi}" ;;
  esac
done

info "Validating BLAKE3 environment variables for required ABIs: ${required_abis[*]}"
for v in "${bootstrap_hash_keys[@]}"; do
  var="TERMUX_BOOTSTRAP_BLAKE3_${v}"
  val="${!var:-}"
  [[ -n "$val" ]] || fail "${var} is not set"
  [[ "$val" =~ ^[0-9a-f]{64}$ ]] || fail "${var}=${val} is not a valid 64-char hex string"
  info "${var}=${val:0:16}..."
  record_diag "- ${var}: ${val:0:16}..."
done

info "Running debug unit tests"
if ./gradlew :app:testDebugUnitTest --no-daemon; then
  record_diag "- app_testDebugUnitTest: success"
else
  test_status=$?
  record_diag "- app_testDebugUnitTest: failure(${test_status})"
  if [[ "${APK_MATRIX_UNIT_TESTS_REQUIRED}" == "true" ]]; then
    fail ":app:testDebugUnitTest failed and APK_MATRIX_UNIT_TESTS_REQUIRED=true"
  fi
  info ":app:testDebugUnitTest failed, but APK_MATRIX_UNIT_TESTS_REQUIRED=${APK_MATRIX_UNIT_TESTS_REQUIRED}; continuing to compile beta APKs"
fi

info "Building unsigned debug and release APKs"
TERMUX_SPLIT_APKS_FOR_DEBUG_BUILDS=1 TERMUX_SPLIT_APKS_FOR_RELEASE_BUILDS=1 ./gradlew :app:assembleDebug :app:assembleRelease --no-daemon
record_diag "- assembleDebug_assembleRelease: success"

cp app/build/outputs/apk/debug/*.apk "${UNSIGNED_DIR}/"
cp app/build/outputs/apk/release/*.apk "${UNSIGNED_DIR}/"

for abi in "${required_abis[@]}"; do
  count=$(find "${UNSIGNED_DIR}" -maxdepth 1 -type f -name "*${abi}*.apk" | wc -l | tr -d ' ')
  [[ "${count}" -gt 0 ]] || fail "${abi} APK was not generated"
  record_diag "- unsigned_${abi}_count: ${count}"
done

if [[ "${KEYSTORE_PATH}" == "${DEFAULT_KEYSTORE_PATH}" ]]; then
  info "Preparing local validation signing material"
  if [[ ! -f "${KEYSTORE_PATH}" ]]; then
    keytool -genkeypair -v -storetype JKS -keystore "${KEYSTORE_PATH}" -alias "${KEY_ALIAS}" -keyalg RSA -keysize 2048 -validity 3650 -storepass "${STORE_PASS}" -keypass "${KEY_PASS}" -dname "CN=Local Build,O=Termux,C=US"
  fi
else
  info "Using provided signing keystore at ${KEYSTORE_PATH}"
fi

[[ -f "${KEYSTORE_PATH}" ]] || fail "keystore file not found at ${KEYSTORE_PATH}"

BUILD_TOOLS_VERSION="$(awk -F= '/^buildToolsVersion=/{gsub(/[[:space:]]/, "", $2); print $2; exit}' gradle.properties || true)"
[[ -n "${BUILD_TOOLS_VERSION}" ]] || BUILD_TOOLS_VERSION="$(awk -F= '/^compileSdkVersion=/{gsub(/[[:space:]]/, "", $2); print $2; exit}' gradle.properties).0.0"
SDK_DIR="$(grep -E '^sdk.dir=' local.properties | cut -d= -f2-)"; SDK_DIR="${SDK_DIR//\\/}"
APKSIGNER="${SDK_DIR}/build-tools/${BUILD_TOOLS_VERSION}/apksigner"
[[ -x "${APKSIGNER}" ]] || fail "apksigner not found at ${APKSIGNER}"

info "Signing release APK variants"
while IFS= read -r -d '' apk; do
  signed_apk="${SIGNED_DIR}/$(basename "${apk%.apk}")-signed.apk"
  "${APKSIGNER}" sign --ks "${KEYSTORE_PATH}" --ks-key-alias "${KEY_ALIAS}" --ks-pass "pass:${STORE_PASS}" --key-pass "pass:${KEY_PASS}" --out "${signed_apk}" "$apk"
  "${APKSIGNER}" verify --print-certs "${signed_apk}" >/dev/null
  echo "signed: ${signed_apk}"
done < <(find "${UNSIGNED_DIR}" -maxdepth 1 -type f -name '*release*.apk' -print0)

declare -A signed_counts=()
for abi in "${required_abis[@]}"; do
  signed_counts[$abi]=$(find "${SIGNED_DIR}" -maxdepth 1 -type f -name "*${abi}*-signed.apk" | wc -l | tr -d ' ')
  [[ "${signed_counts[$abi]}" -gt 0 ]] || fail "signed ${abi} release APK missing"
  record_diag "- signed_${abi}_count: ${signed_counts[$abi]}"
done

if [[ "${RELEASE_TRACK}" == "official" ]]; then
  info "Official track: removing unsigned release APKs from publishable artifact tree after signing"
  find "${UNSIGNED_DIR}" -maxdepth 1 -type f -name "*release*.apk" -delete
fi

# Build reports after any track-specific pruning so upload contract and hashes
# describe exactly the publishable artifact tree.
( cd "${OUT_DIR}" && find unsigned signed -type f -name '*.apk' -print0 | xargs -0 sha256sum > SHA256SUMS.txt )

SIZE_REPORT="${OUT_DIR}/APK_SIZE_REPORT.tsv"
printf 'apk\ttype\tabi\tsize_bytes\n' > "${SIZE_REPORT}"
while IFS= read -r -d '' apk; do
  base="$(basename "$apk")"
  size="$(stat -c '%s' "$apk")"
  abi="$(detect_apk_abi "$base")"
  kind="unsigned"
  [[ "$apk" == *"/signed/"* ]] && kind="signed"
  printf '%s\t%s\t%s\t%s\n' "$base" "$kind" "$abi" "$size" >> "${SIZE_REPORT}"
done < <(find "${OUT_DIR}" -type f -name '*.apk' -print0)

DIFF_REPORT="${OUT_DIR}/APK_SIZE_DIFF_RELEASE.tsv"
printf 'abi\tunsigned_apk\tunsigned_size_bytes\tsigned_apk\tsigned_size_bytes\tdelta_bytes\n' > "${DIFF_REPORT}"
for abi in "${required_abis[@]}" universal; do
  uapk="$(find "${UNSIGNED_DIR}" -maxdepth 1 -type f -name "*release*${abi}*.apk" | head -n1 || true)"
  sapk="$(find "${SIGNED_DIR}" -maxdepth 1 -type f -name "*release*${abi}*-signed.apk" | head -n1 || true)"
  if [[ -n "$uapk" && -n "$sapk" ]]; then
    usize="$(stat -c '%s' "$uapk")"
    ssize="$(stat -c '%s' "$sapk")"
    delta=$((ssize-usize))
    printf '%s\t%s\t%s\t%s\t%s\t%s\n' "$abi" "$(basename "$uapk")" "$usize" "$(basename "$sapk")" "$ssize" "$delta" >> "${DIFF_REPORT}"
  fi
done

( cd "${OUT_DIR}" && {
  echo "artifact_dir=${OUT_DIR}";
  echo "release_track=${RELEASE_TRACK}";
  echo "generated_at_utc=$(date -u +%Y-%m-%dT%H:%M:%SZ)";
  echo "signing_keystore=$(basename "${KEYSTORE_PATH}")";
  cat dist/source-contract/TERMUX_PACKAGES_RAFCODEPHI_SOURCE.env 2>/dev/null || true;
  echo "signed_release_apks_required=$(printf "%s " "${required_abis[@]}")";
  find unsigned signed -type f -name '*.apk' | sort;
} > ARTIFACT_MANIFEST.txt )
record_diag "- artifact_manifest: success"
info "Artifacts generated in ${OUT_DIR}"
cat "${OUT_DIR}/ARTIFACT_MANIFEST.txt"
cat "${OUT_DIR}/APK_MATRIX_DIAGNOSTIC.md"
cat "${OUT_DIR}/APK_SIZE_REPORT.tsv"
cat "${OUT_DIR}/APK_SIZE_DIFF_RELEASE.tsv"
cat "${OUT_DIR}/SHA256SUMS.txt"
