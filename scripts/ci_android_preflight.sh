#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="${1:-.}"

"${ROOT_DIR}/scripts/setup_android_toolchain.sh"
"${ROOT_DIR}/scripts/ensure_android_sdk.sh" "${ROOT_DIR}"

if [[ -f "${ROOT_DIR}/gradlew" ]]; then
  chmod +x "${ROOT_DIR}/gradlew"
fi

echo "✅ Android CI preflight concluído (toolchain + sdk.dir + gradlew)."
