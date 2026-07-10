#!/usr/bin/env bash
# run_readonly_flow.sh — Vertical Slice v1
# Pipeline: validate intent_ir -> governance gate -> build plan -> run git -> execution_result.json
#
# Usage:
#   bash scripts/vertical_slice/run_readonly_flow.sh <intent_ir.json> [working_directory]
#
# Outputs: execution_result.json in current directory
# Requirements: bash 4+, python3, git, sha256sum (or shasum -a 256)
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"
export INTENT_SCHEMA="${REPO_ROOT}/docs/contracts/intent_ir.schema.json"
export CAPABILITIES="${REPO_ROOT}/internal/governance/capabilities.json"

# ─────────────────────────────────────────
# 0. Arguments
# ─────────────────────────────────────────
export INTENT_FILE="${1:-}"
export WORK_DIR="${2:-$(pwd)}"

if [[ -z "${INTENT_FILE}" ]]; then
  echo "[ERROR] Usage: $0 <intent_ir.json> [working_directory]" >&2
  exit 1
fi
if [[ ! -f "${INTENT_FILE}" ]]; then
  echo "[ERROR] Intent file not found: ${INTENT_FILE}" >&2
  exit 1
fi
if [[ ! -d "${WORK_DIR}" ]]; then
  echo "[ERROR] Working directory not found: ${WORK_DIR}" >&2
  exit 1
fi

echo "[INFO] intent_ir:    ${INTENT_FILE}"
echo "[INFO] working_dir:  ${WORK_DIR}"

# ─────────────────────────────────────────
# 1. Validate intent_ir against schema
# ─────────────────────────────────────────
echo "[STEP 1] Validating intent_ir schema..."

python3 -c "
import sys, json, os, re

intent_file  = os.environ['INTENT_FILE']
schema_file  = os.environ.get('INTENT_SCHEMA', '')

with open(intent_file) as f:
    intent = json.load(f)

REQUIRED = ['schema','intent_id','action','target','inputs','constraints',
            'evidence_refs','requested_capabilities','risk','execution_gate']
missing = [k for k in REQUIRED if k not in intent]
if missing:
    print(f'[FAIL] Missing required fields: {missing}', file=sys.stderr)
    sys.exit(2)
if intent.get('schema') != 'rafaelia.intent.v1':
    print(f'[FAIL] schema must be rafaelia.intent.v1, got: {intent.get(\"schema\")!r}', file=sys.stderr)
    sys.exit(2)
if intent.get('risk') not in ('low','medium','high','critical'):
    print(f'[FAIL] invalid risk: {intent.get(\"risk\")!r}', file=sys.stderr)
    sys.exit(2)
if intent.get('execution_gate') not in ('allow','sandbox_only','human_review','blocked'):
    print(f'[FAIL] invalid execution_gate: {intent.get(\"execution_gate\")!r}', file=sys.stderr)
    sys.exit(2)

try:
    import jsonschema
    if os.path.exists(schema_file):
        with open(schema_file) as sf:
            schema = json.load(sf)
        jsonschema.validate(intent, schema)
        print('[OK] Schema validated with jsonschema')
    else:
        print('[OK] Schema validated (minimal)')
except ImportError:
    print('[OK] Schema validated (jsonschema not installed, minimal checks passed)')
except Exception as e:
    print(f'[FAIL] jsonschema: {e}', file=sys.stderr)
    sys.exit(2)
"

# ─────────────────────────────────────────
# 2. Governance gate
# ─────────────────────────────────────────
echo "[STEP 2] Applying governance gate..."

GATE_RESULT="$(python3 -c "
import sys, json, os

intent_file = os.environ['INTENT_FILE']
caps_file   = os.environ['CAPABILITIES']

with open(intent_file) as f:
    intent = json.load(f)
with open(caps_file) as f:
    caps_doc = json.load(f)

allowed_ids  = {c['id'] for c in caps_doc['capabilities']}
V1_ALLOWED   = {'git.read', 'git.diff'}

if intent.get('execution_gate') == 'blocked':
    print('BLOCKED:execution_gate=blocked'); sys.exit(0)
if intent.get('risk') == 'critical':
    print('BLOCKED:risk=critical'); sys.exit(0)
if intent.get('execution_gate') == 'human_review' or intent.get('risk') == 'high':
    print('HUMAN_REVIEW:manual approval required'); sys.exit(0)

requested = intent.get('requested_capabilities', [])
unknown = [c for c in requested if c not in allowed_ids]
if unknown:
    print(f'BLOCKED:unknown capabilities {unknown}'); sys.exit(0)
non_v1 = [c for c in requested if c not in V1_ALLOWED]
if non_v1:
    print(f'BLOCKED:v1 does not allow {non_v1}'); sys.exit(0)
if intent.get('execution_gate') != 'allow':
    print(f'HUMAN_REVIEW:execution_gate={intent.get(\"execution_gate\")}'); sys.exit(0)

print(f'ALLOW:capabilities={requested}')
")"

GATE_DECISION="${GATE_RESULT%%:*}"
GATE_REASON="${GATE_RESULT#*:}"

echo "[GATE] ${GATE_DECISION} — ${GATE_REASON}"

if [[ "${GATE_DECISION}" == "BLOCKED" ]]; then
  echo "[ABORT] Gate blocked execution." >&2
  exit 3
fi
if [[ "${GATE_DECISION}" == "HUMAN_REVIEW" ]]; then
  echo "[ABORT] Gate requires human review." >&2
  exit 4
fi

# ─────────────────────────────────────────
# 3. Build execution plan
# ─────────────────────────────────────────
echo "[STEP 3] Building execution plan..."

export INTENT_ID="$(python3 -c "import json,os; print(json.load(open(os.environ['INTENT_FILE']))['intent_id'])")"
export PLAN_ID="plan-$(date +%s)-${INTENT_ID:0:8}"
export STARTED_AT="$(date -u +%Y-%m-%dT%H:%M:%SZ)"
echo "[INFO] plan_id=${PLAN_ID}  intent_id=${INTENT_ID}  started_at=${STARTED_AT}"

# ─────────────────────────────────────────
# 4. Execute git status + git diff --stat
# ─────────────────────────────────────────
echo "[STEP 4a] git status..."
export GIT_STATUS_OUT
export GIT_STATUS_ERR
GIT_STATUS_OUT="$(git -C "${WORK_DIR}" status 2>/tmp/vs_err_status || true)"
GIT_STATUS_ERR="$(cat /tmp/vs_err_status 2>/dev/null || true)"
echo "${GIT_STATUS_OUT}" | head -20

echo "[STEP 4b] git diff --stat..."
export GIT_DIFF_OUT
export GIT_DIFF_ERR
GIT_DIFF_OUT="$(git -C "${WORK_DIR}" diff --stat 2>/tmp/vs_err_diff || true)"
GIT_DIFF_ERR="$(cat /tmp/vs_err_diff 2>/dev/null || true)"
echo "${GIT_DIFF_OUT}" | head -20

# ─────────────────────────────────────────
# 5 & 6. Compute sha256 hashes
# ─────────────────────────────────────────
echo "[STEP 5/6] Computing sha256 hashes..."

sha256_of() {
  printf '%s' "$1" | python3 -c "import sys,hashlib; print(hashlib.sha256(sys.stdin.buffer.read()).hexdigest())"
}

export STDOUT_STATUS_SHA="$(sha256_of "${GIT_STATUS_OUT}")"
export STDERR_STATUS_SHA="$(sha256_of "${GIT_STATUS_ERR}")"
export STDOUT_DIFF_SHA="$(sha256_of "${GIT_DIFF_OUT}")"
export STDERR_DIFF_SHA="$(sha256_of "${GIT_DIFF_ERR}")"
export ENDED_AT="$(date -u +%Y-%m-%dT%H:%M:%SZ)"

echo "[INFO] stdout_sha256(status) = ${STDOUT_STATUS_SHA}"
echo "[INFO] stdout_sha256(diff)   = ${STDOUT_DIFF_SHA}"

# ─────────────────────────────────────────
# 7. Generate execution_result.json
# ─────────────────────────────────────────
echo "[STEP 7] Generating execution_result.json..."

export RESULT_ID="result-$(date +%s)-${INTENT_ID:0:8}"
export SOURCE_CHUNKS="$(python3 -c "
import json,os
intent = json.load(open(os.environ['INTENT_FILE']))
refs = [e['chunk_id'] for e in intent.get('evidence_refs', [])]
print(json.dumps(refs))
")"

python3 -c "
import json, os

result = {
    'result_id':          os.environ['RESULT_ID'],
    'intent_id':          os.environ['INTENT_ID'],
    'plan_id':            os.environ['PLAN_ID'],
    'executed_command':   'git',
    'args':               ['status', 'AND', 'diff', '--stat'],
    'working_directory':  os.environ['WORK_DIR'],
    'started_at':         os.environ['STARTED_AT'],
    'ended_at':           os.environ['ENDED_AT'],
    'exit_code':          0,
    'stdout_truncated':  (
        '=== git status ===\n' + os.environ.get('GIT_STATUS_OUT','') +
        '\n\n=== git diff --stat ===\n' + os.environ.get('GIT_DIFF_OUT','')
    )[:4096],
    'stderr_truncated':  (os.environ.get('GIT_STATUS_ERR','') + os.environ.get('GIT_DIFF_ERR',''))[:4096],
    'stdout_sha256':     os.environ['STDOUT_STATUS_SHA'],
    'stderr_sha256':     os.environ['STDERR_STATUS_SHA'],
    'artifacts': [
        {'name': 'git_status_stdout',    'path': '/dev/stdout', 'sha256': os.environ['STDOUT_STATUS_SHA']},
        {'name': 'git_diff_stat_stdout', 'path': '/dev/stdout', 'sha256': os.environ['STDOUT_DIFF_SHA']},
    ],
    'final_state':        'success',
    'rollback_available': False,
    'source_chunk_refs':  json.loads(os.environ.get('SOURCE_CHUNKS','[]')),
}

out = os.path.join(os.getcwd(), 'execution_result.json')
json.dump(result, open(out, 'w'), indent=2)
print(f'[OK] execution_result.json -> {out}')
"

echo ""
echo "[DONE] Vertical slice v1 complete."
echo "  Result: $(pwd)/execution_result.json"
