#!/usr/bin/env bash
# Static checks to avoid upstream conflicts (no adb)
set -euo pipefail

APP_ID_EXPECT='com.termux.rafacodephi'

# Check applicationId in app/build.gradle
if ! grep -R "applicationId \"$APP_ID_EXPECT\"" -n app/build.gradle >/dev/null 2>&1; then
  echo "FAIL: applicationId not set to $APP_ID_EXPECT in app/build.gradle";
  exit 1
fi

# Check that authorities mention rafacodephi, not com.termux
if grep -R "com\.termux\.files" -n app/src/main/AndroidManifest.xml >/dev/null 2>&1; then
  echo "WARN: possible authority conflict in AndroidManifest.xml (com.termux.files)";
fi

echo "OK: upstream-parallel static checks";
