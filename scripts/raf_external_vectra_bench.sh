#!/usr/bin/env bash
set -euo pipefail

vectra_path="${RAF_VECTRA_PATH:-}"
strict_mode="${RAF_STRICT_MODE:-debug}"
require_vectra="${RAF_REQUIRE_VECTRA:-false}"
reports_dir="reports"

while [[ $# -gt 0 ]]; do
  case "$1" in
    --vectra-path) vectra_path="$2"; shift 2 ;;
    *) echo "Unknown arg: $1" >&2; exit 2 ;;
  esac
done

mkdir -p "$reports_dir"
status="warning_missing"
message="vectra_rmr not found; debug fallback allowed"

strict="false"
if [[ "$strict_mode" == "release" || "${CI:-}" == "true" || "$require_vectra" == "true" ]]; then
  strict="true"
fi

if [[ -n "$vectra_path" && -e "$vectra_path" ]]; then
  status="ok_present"
  message="vectra_rmr integration path detected"
else
  if [[ "$strict" == "true" ]]; then
    status="error_missing"
    message="vectra_rmr is required in strict mode"
  fi
fi

cat > "$reports_dir/vectra_integration_report.json" <<JSON
{
  "technical_name": "termux_rafcodephi",
  "vectra_path": "$vectra_path",
  "status": "$status",
  "strict": $strict,
  "message": "$message"
}
JSON

cat > "$reports_dir/vectra_integration_report.md" <<MD
# Vectra Integration Report

- vectra_path: $vectra_path
- status: $status
- strict: $strict
- message: $message
MD

if [[ "$status" == "error_missing" ]]; then
  echo "ERROR: $message" >&2
  exit 1
fi

echo "OK: vectra integration report generated"
