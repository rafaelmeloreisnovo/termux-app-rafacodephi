#!/usr/bin/env sh
set -u

OUT="${1:-artifacts/termux-runtime-evidence.json}"
APK_PATH="${APK_PATH:-}"
mkdir -p "$(dirname "$OUT")"

escape_json() {
  printf '%s' "$1" | sed 's/\\/\\\\/g; s/"/\\"/g; s/[[:cntrl:]]/ /g'
}

observed_at="$(date -u +%Y-%m-%dT%H:%M:%SZ 2>/dev/null || printf TOKEN_VAZIO)"
abi="$(getprop ro.product.cpu.abi 2>/dev/null || true)"
[ -n "$abi" ] || abi="$(uname -m 2>/dev/null || printf TOKEN_VAZIO)"
prefix_value="${PREFIX:-TOKEN_VAZIO}"

apk_sha="TOKEN_VAZIO"
if [ -n "$APK_PATH" ] && [ -f "$APK_PATH" ] && command -v sha256sum >/dev/null 2>&1; then
  apk_sha="$(sha256sum "$APK_PATH" | awk '{print $1}')"
fi

probe() {
  name="$1"
  path="$(command -v "$name" 2>/dev/null || true)"
  exists=false
  executable=false
  exit_code=127
  version="TOKEN_VAZIO"
  if [ -n "$path" ]; then
    exists=true
    [ -x "$path" ] && executable=true
    case "$name" in
      sh) version="$($path -c 'printf shell-ok' 2>/dev/null)"; exit_code=$? ;;
      ls) version="$($path --version 2>/dev/null | head -n 1)"; exit_code=$? ;;
      pkg|apt|dpkg|proot) version="$($path --version 2>/dev/null | head -n 1)"; exit_code=$? ;;
    esac
    [ -n "$version" ] || version="PRESENT_VERSION_TOKEN_VAZIO"
  fi
  printf '{"name":"%s","path":"%s","exists":%s,"executable":%s,"probe_exit_code":%s,"version":"%s"}' \
    "$(escape_json "$name")" "$(escape_json "$path")" "$exists" "$executable" "$exit_code" "$(escape_json "$version")"
}

has_pkg=false; command -v pkg >/dev/null 2>&1 && has_pkg=true
has_apt=false; command -v apt >/dev/null 2>&1 && has_apt=true
has_dpkg=false; command -v dpkg >/dev/null 2>&1 && has_dpkg=true
has_proot=false; command -v proot >/dev/null 2>&1 && has_proot=true

backend_status="TOKEN_VAZIO"
if [ "$has_apt" = true ] && [ "$has_dpkg" = true ]; then
  backend_status="PRESENT_UNVERIFIED"
elif [ "$has_pkg" = true ]; then
  backend_status="WRAPPER_ONLY_OR_INCOMPLETE"
fi

{
  printf '{\n'
  printf '  "schema": "termux.rafacodephi.runtime_evidence.v1",\n'
  printf '  "claim_allowed": false,\n'
  printf '  "observed_at": "%s",\n' "$(escape_json "$observed_at")"
  printf '  "device_abi": "%s",\n' "$(escape_json "$abi")"
  printf '  "prefix": "%s",\n' "$(escape_json "$prefix_value")"
  printf '  "apk_sha256": "%s",\n' "$apk_sha"
  printf '  "package_backend_status": "%s",\n' "$backend_status"
  printf '  "proot_present": %s,\n' "$has_proot"
  printf '  "commands": ['
  first=true
  for cmd in sh ls pkg apt dpkg proot; do
    [ "$first" = true ] || printf ','
    printf '\n    '
    probe "$cmd"
    first=false
  done
  printf '\n  ],\n'
  printf '  "runtime_status": "OBSERVED_LOCAL",\n'
  printf '  "install_or_mutation_performed": false,\n'
  printf '  "rollback": "No mutation was performed; retain previous APK hash and data backup.",\n'
  printf '  "next_action": "Run on the target Android device and attach this JSON plus APK hash and command logs."\n'
  printf '}\n'
} > "$OUT"

printf '%s\n' "$OUT"
