#!/usr/bin/env bash
set -euo pipefail

root=".github/workflows"
status=0
warning_count=0
strict_mode="${CI_METADATA_STRICT:-0}"

if [[ ! -d "$root" ]]; then
  echo "[ERROR] workflow directory not found: $root"
  exit 1
fi

shopt -s nullglob
workflows=("$root"/*.yml "$root"/*.yaml)
shopt -u nullglob

if [[ ${#workflows[@]} -eq 0 ]]; then
  echo "[ERROR] no GitHub Actions workflow files found in $root"
  exit 1
fi

for wf in "${workflows[@]}"; do
  track=$(sed -n 's/^# ci_track:[[:space:]]*//p' "$wf" | head -n1 | tr -d '\r')
  abis=$(sed -n 's/^# ci_abis:[[:space:]]*//p' "$wf" | head -n1 | tr -d '\r')

  if [[ -z "$track" ]]; then
    if [[ "$strict_mode" == "1" ]]; then
      echo "[ERROR] missing # ci_track in $wf"
      status=1
    else
      echo "[WARN] missing # ci_track in $wf; defaulting to ops"
      track="ops"
      warning_count=$((warning_count + 1))
    fi
  fi

  if [[ -z "$abis" ]]; then
    if [[ "$strict_mode" == "1" ]]; then
      echo "[ERROR] missing # ci_abis in $wf"
      status=1
    else
      echo "[WARN] missing # ci_abis in $wf; defaulting to n/a"
      abis="n/a"
      warning_count=$((warning_count + 1))
    fi
  fi

  if [[ -n "$track" ]]; then
    case "$track" in
      debug|internal|official|artifact|ops|deprecated) ;;
      *)
        echo "[ERROR] invalid ci_track '$track' in $wf"
        status=1
        ;;
    esac
  fi

  if [[ -n "$abis" ]]; then
    case "$abis" in
      n/a|none|all|armeabi-v7a|arm64-v8a|x86|x86_64|*,*) ;;
      *)
        echo "[ERROR] invalid ci_abis '$abis' in $wf"
        status=1
        ;;
    esac
  fi

done

if [[ $status -ne 0 ]]; then
  exit $status
fi

if [[ $warning_count -gt 0 ]]; then
  echo "[OK] workflow metadata contract validated with $warning_count compatibility warning(s) (${#workflows[@]} workflows)"
else
  echo "[OK] workflow metadata contract validated (${#workflows[@]} workflows)"
fi
