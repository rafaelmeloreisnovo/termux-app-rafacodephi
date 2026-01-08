#!/usr/bin/env bash
set -euo pipefail

SAFE_PREFIX="$(pwd)/"

ensure_safe_path() {
  local path="$1"
  if [[ -z "${path}" || "${path}" == "/" || "${path}" == "." ]]; then
    echo "Unsafe path: '${path}'" >&2
    exit 1
  fi
  local normalized
  normalized="$(realpath -m "${path}")"
  if [[ ${#normalized} -lt 5 ]]; then
    echo "Path too short: ${normalized}" >&2
    exit 1
  fi
  case "${normalized}" in
    "${SAFE_PREFIX}"*)
      ;;
    *)
      echo "Path outside allowed prefix: ${normalized}" >&2
      exit 1
      ;;
  esac
}

safe_mv() {
  local src="$1"
  local dst="$2"
  ensure_safe_path "${src}"
  ensure_safe_path "${dst}"
  mv "${src}" "${dst}"
}

CORE="$HOME/RAFAELIA_CORE"
DEC="$CORE/DECISION_OMEGA.json"
while true; do
  ACTION=$(jq -r '.next_action // empty' "$DEC")
  if [ -n "$ACTION" ]; then
    echo "[⚡️] Executando: $ACTION"
    eval "$ACTION"
    jq 'del(.next_action)' "$DEC" > tmp && safe_mv tmp "$DEC"
  fi
  sleep 10
done
