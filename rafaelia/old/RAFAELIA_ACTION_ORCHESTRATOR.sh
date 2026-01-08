#!/usr/bin/env bash
set -euo pipefail

CORE="$HOME/RAFAELIA_CORE"
SAFE_PREFIX="${CORE%/}/"

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
    if [[ "$ACTION" =~ [^A-Za-z0-9_./:=@-] ]]; then
      echo "Unsafe characters in action; skipping" >&2
    else
      read -r -a action_parts <<< "$ACTION"
      if command -v "${action_parts[0]}" >/dev/null 2>&1; then
        "${action_parts[@]}"
      else
        echo "Unknown action command: ${action_parts[0]}" >&2
      fi
    fi
    tmp_file="$(mktemp "${CORE}/.next_action.XXXXXX")"
    jq 'del(.next_action)' "$DEC" > "${tmp_file}" && safe_mv "${tmp_file}" "$DEC"
  fi
  sleep 10
done
