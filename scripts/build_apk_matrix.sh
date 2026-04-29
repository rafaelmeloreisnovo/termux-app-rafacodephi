#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
OUT_DIR="${ROOT_DIR}/dist/apk-matrix"
KEYSTORE_PATH="${ROOT_DIR}/dist/local-release.keystore"
KEY_ALIAS="localrelease"
KEY_PASS="${KEY_PASS:-changeit}"
STORE_PASS="${STORE_PASS:-changeit}"

cd "${ROOT_DIR}"
./scripts/ensure_android_sdk.sh "${ROOT_DIR}"
./scripts/ci_android_preflight.sh
mkdir -p "${OUT_DIR}" "$(dirname "${KEYSTORE_PATH}")"

./gradlew :app:downloadBootstraps

if ! python3 -c 'import blake3' >/dev/null 2>&1; then
  python3 -m pip install --user blake3 >/dev/null
fi

read -r TERMUX_BOOTSTRAP_BLAKE3_AARCH64 TERMUX_BOOTSTRAP_BLAKE3_ARM TERMUX_BOOTSTRAP_BLAKE3_I686 TERMUX_BOOTSTRAP_BLAKE3_X86_64 < <(
python3 - <<'PY'
from pathlib import Path
from blake3 import blake3
base = Path('app/src/main/cpp')
files = ['bootstrap-aarch64.zip','bootstrap-arm.zip','bootstrap-i686.zip','bootstrap-x86_64.zip']
print(' '.join(blake3((base/f).read_bytes()).hexdigest() for f in files))
PY
)
export TERMUX_BOOTSTRAP_BLAKE3_AARCH64 TERMUX_BOOTSTRAP_BLAKE3_ARM TERMUX_BOOTSTRAP_BLAKE3_I686 TERMUX_BOOTSTRAP_BLAKE3_X86_64

TERMUX_BOOTSTRAP_VALIDATION_MODE=upstream-debug-compat ./gradlew :app:assembleDebug
./gradlew :app:assembleRelease

cp app/build/outputs/apk/debug/*arm64-v8a*.apk "${OUT_DIR}/" || true
cp app/build/outputs/apk/debug/*armeabi-v7a*.apk "${OUT_DIR}/" || true
cp app/build/outputs/apk/release/*.apk "${OUT_DIR}/" || true

if [[ ! -f "${KEYSTORE_PATH}" ]]; then
  keytool -genkeypair -v -storetype JKS -keystore "${KEYSTORE_PATH}" -alias "${KEY_ALIAS}" -keyalg RSA -keysize 2048 -validity 3650 -storepass "${STORE_PASS}" -keypass "${KEY_PASS}" -dname "CN=Local Build,O=Termux,C=US"
fi

BUILD_TOOLS_VERSION="$(grep -E '^buildToolsVersion=' gradle.properties | cut -d= -f2 | tr -d '[:space:]')"
[[ -n "${BUILD_TOOLS_VERSION}" ]] || BUILD_TOOLS_VERSION="$(grep -E '^compileSdkVersion=' gradle.properties | cut -d= -f2 | tr -d '[:space:]').0.0"
SDK_DIR="$(grep -E '^sdk.dir=' local.properties | cut -d= -f2-)"; SDK_DIR="${SDK_DIR//\\/}"
APKSIGNER="${SDK_DIR}/build-tools/${BUILD_TOOLS_VERSION}/apksigner"

for apk in "${OUT_DIR}"/*.apk; do
  [[ -e "$apk" ]] || continue
  signed_apk="${apk%.apk}-signed.apk"
  "${APKSIGNER}" sign --ks "${KEYSTORE_PATH}" --ks-key-alias "${KEY_ALIAS}" --ks-pass "pass:${STORE_PASS}" --key-pass "pass:${KEY_PASS}" --out "${signed_apk}" "$apk"
  "${APKSIGNER}" verify --print-certs "${signed_apk}"
done

( cd "${OUT_DIR}" && sha256sum *.apk > SHA256SUMS.txt )
echo "Artifacts generated in ${OUT_DIR}"
