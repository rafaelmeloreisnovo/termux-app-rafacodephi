#!/bin/sh
# S15 physical inventory ONLY. No install, uninstall, package write or private logs.
set -eu
: "${S15_DEVICE_SERIAL:?BLOCKED: exact authorized adb device serial required}"
command -v adb >/dev/null || { echo 'BLOCKED: adb unavailable' >&2; exit 3; }
test "$(adb -s "$S15_DEVICE_SERIAL" get-state)" = 'device' ||
  { echo 'BLOCKED: device unavailable or unauthorized' >&2; exit 4; }
abi=$(adb -s "$S15_DEVICE_SERIAL" shell getprop ro.product.cpu.abi | tr -d '\r\n')
sdk=$(adb -s "$S15_DEVICE_SERIAL" shell getprop ro.build.version.sdk | tr -d '\r\n')
case "$abi" in armeabi-v7a|arm64-v8a) ;; *)
  echo "BLOCKED: unsupported active ABI=$abi" >&2; exit 5;;
esac
case "$sdk" in ''|*[!0-9]*) echo 'BLOCKED: invalid SDK field' >&2; exit 5;; esac
printf 'S15_OBSERVATION_DEVICE_ABI=%s\n' "$abi"
printf 'S15_OBSERVATION_SDK=%s\n' "$sdk"
pkg=$(adb -s "$S15_DEVICE_SERIAL" shell pm path com.termux.rafacodephi | tr -d '\r')
case "$pkg" in package:*) echo 'S15_TERMUX_PACKAGE=INSTALLED_METADATA_ONLY';;
  *) echo 'S15_TERMUX_PACKAGE=TOKEN_VAZIO_NOT_INSTALLED_OR_NOT_VISIBLE';;
esac
echo 'S15_ANDROID_PLAN_EXECUTION=TOKEN_VAZIO_NOT_RUN'
echo 'S15_APK_INSTALLATION_AND_KEYBOARD=TOKEN_VAZIO_NOT_TESTED'
echo 'S15_NO_DEVICE_MUTATION=TRUE'
