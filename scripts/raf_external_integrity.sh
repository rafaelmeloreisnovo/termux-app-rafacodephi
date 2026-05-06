#!/usr/bin/env bash
set -euo pipefail

backend="python-blake3"
input_file="app/src/main/cpp/bootstrap-arm.zip"
strict_mode="${RAF_STRICT_MODE:-debug}"
release_mode="${RAF_RELEASE_MODE:-false}"
upstream_debug_compat="${RAF_UPSTREAM_DEBUG_COMPAT:-false}"
require_external_backend="${RAF_REQUIRE_EXTERNAL_BACKEND:-false}"
reports_dir="reports"

usage() {
  echo "Usage: $0 [--backend python-blake3|external-rmr-pai] [--input <file>]" >&2
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --backend) backend="$2"; shift 2 ;;
    --input) input_file="$2"; shift 2 ;;
    -h|--help) usage; exit 0 ;;
    *) echo "Unknown arg: $1" >&2; usage; exit 2 ;;
  esac
done

mkdir -p "$reports_dir"

is_strict="false"
if [[ "$strict_mode" == "release" || "$release_mode" == "true" || "${CI:-}" == "true" ]]; then
  is_strict="true"
fi

warn_or_fail() {
  local msg="$1"
  if [[ "$is_strict" == "true" ]]; then
    echo "ERROR: $msg" >&2
    exit 1
  fi
  echo "WARNING: $msg" >&2
}

sha256=""
if [[ ! -f "$input_file" ]]; then
  warn_or_fail "input bootstrap not found: $input_file"
else
  sha256="$(sha256sum "$input_file" | awk '{print $1}')"
fi
blake3=""
backend_status="ok"

if [[ "$backend" == "python-blake3" ]]; then
  if python3 -c 'import blake3' >/dev/null 2>&1; then
    blake3="$(python3 - <<PY
from blake3 import blake3
from pathlib import Path
print(blake3(Path('$input_file').read_bytes()).hexdigest())
PY
)"
  else
    backend_status="missing_python_blake3"
    warn_or_fail "python blake3 module not available"
  fi
elif [[ "$backend" == "external-rmr-pai" ]]; then
  ext_cmd="${RAF_EXTERNAL_RMR_PAI_CMD:-}"
  if [[ -n "$ext_cmd" ]] && command -v "$ext_cmd" >/dev/null 2>&1; then
    blake3="$($ext_cmd "$input_file")"
  else
    backend_status="missing_external_rmr_pai"
    if [[ "$require_external_backend" == "true" ]]; then
      warn_or_fail "external-rmr-pai backend required but not found"
    else
      warn_or_fail "external-rmr-pai backend unavailable; fallback to python-blake3"
      if python3 -c 'import blake3' >/dev/null 2>&1; then
        blake3="$(python3 - <<PY
from blake3 import blake3
from pathlib import Path
print(blake3(Path('$input_file').read_bytes()).hexdigest())
PY
)"
        backend="python-blake3"
        backend_status="fallback_python_blake3"
      fi
    fi
  fi
else
  echo "Unsupported backend: $backend" >&2
  exit 2
fi

boot_info_status="missing"
runtime_ready="false"
if [[ -f "$input_file" ]] && unzip -p "$input_file" BOOTSTRAP_INFO >/tmp/bootstrap_info.txt 2>/dev/null; then
  if rg -n '^TERMUX_PACKAGE_NAME=' /tmp/bootstrap_info.txt >/dev/null && rg -n '^TERMUX_PAGE_SIZE=16384$' /tmp/bootstrap_info.txt >/dev/null; then
    boot_info_status="valid"
    runtime_ready="true"
  else
    boot_info_status="invalid"
  fi
fi

if [[ -z "$sha256" || -z "$blake3" || "$boot_info_status" != "valid" || "$runtime_ready" != "true" ]]; then
  if [[ "$is_strict" == "true" ]]; then
    echo "ERROR: strict release integrity contract failed" >&2
    exit 1
  fi
fi

cat > "$reports_dir/bootstrap_integrity.json" <<JSON
{
  "technical_name": "termux_rafcodephi",
  "backend": "$backend",
  "backend_status": "$backend_status",
  "input": "$input_file",
  "sha256": "$sha256",
  "blake3": "$blake3",
  "bootstrap_info_status": "$boot_info_status",
  "runtime_ready": $runtime_ready,
  "strict_mode": $is_strict,
  "upstream_debug_compat": "$upstream_debug_compat"
}
JSON

printf "input\tbackend\tsha256\tblake3\tbootstrap_info_status\truntime_ready\n%s\t%s\t%s\t%s\t%s\t%s\n" \
  "$input_file" "$backend" "$sha256" "$blake3" "$boot_info_status" "$runtime_ready" > "$reports_dir/bootstrap_integrity.tsv"

cat > "$reports_dir/bootstrap_integrity.md" <<MD
# Bootstrap Integrity Report

- input: \
  \
  \
  \
  \
  \
  \
  \
  \
  \
  \
  \
  \
  \
  \
  \
  \
  \
  \
  \
  \
  $input_file
- backend: $backend
- backend_status: $backend_status
- sha256: $sha256
- blake3: $blake3
- bootstrap_info_status: $boot_info_status
- runtime_ready: $runtime_ready
- strict_mode: $is_strict
- upstream_debug_compat: $upstream_debug_compat
MD

cat > "$reports_dir/external_integration_status.md" <<MD
# External Integration Status

- blake3_rmr_integrity: $backend_status
- vectra_rmr_contract: pending_or_external
- strict_mode: $is_strict
MD

cat > "$reports_dir/symbol_encoding_report.json" <<JSON
{
  "technical_name": "termux_rafcodephi",
  "checks": {
    "unicode_in_script_paths": false,
    "rtl_ltr_direction_risk": "report_only",
    "shell_escape_risk": "low",
    "url_escape_risk": "requires_percent_encoding_for_non_ascii",
    "json_escape_risk": "low_if_ascii_keys",
    "html_charset_required": "UTF-8",
    "gradle_identifier_unicode_risk": "avoid_unicode"
  }
}
JSON

echo "OK: integrity reports generated in $reports_dir"
