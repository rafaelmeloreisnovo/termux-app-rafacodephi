#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT_DIR"

status=0

check_cmd() {
  local name="$1"
  local cmd="$2"
  if command -v "$cmd" >/dev/null 2>&1; then
    echo "✅ $name: $(command -v "$cmd")"
  else
    echo "❌ $name: missing ($cmd)"
    status=1
  fi
}

echo "== RAFAELIA Doctor =="
check_cmd "Java" java
check_cmd "Javac" javac
check_cmd "Python3" python3
check_cmd "Git" git

if [[ -n "${ANDROID_HOME:-}" || -n "${ANDROID_SDK_ROOT:-}" ]]; then
  echo "✅ Android SDK env configured"
else
  echo "⚠️ Android SDK env not configured (ANDROID_HOME/ANDROID_SDK_ROOT)"
  status=1
fi

if [[ -f local.properties ]] && grep -q '^sdk.dir=' local.properties; then
  echo "✅ local.properties sdk.dir configured"
else
  echo "⚠️ local.properties sdk.dir missing"
fi

if [[ -x ./gradlew ]]; then
  echo "✅ gradlew present"
else
  echo "❌ gradlew missing or not executable"
  status=1
fi

exit $status
