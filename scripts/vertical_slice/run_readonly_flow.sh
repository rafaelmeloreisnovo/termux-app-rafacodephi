#!/usr/bin/env bash
# run_readonly_flow.sh — Vertical Slice v1, receipt-hardening revision.
# Pipeline: validate intent_ir -> governance gate -> read-only git probe ->
#           immutable execution_result.json.
#
# Usage:
#   bash scripts/vertical_slice/run_readonly_flow.sh \
#     <intent_ir.json> [working_directory] [result_root]
#
# The result root must be outside the working directory. Each invocation creates
# one unique subdirectory and never replaces a prior receipt.
# Requirements: bash 4+, python3, git.
set -euo pipefail
umask 077

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd -P)"
export INTENT_SCHEMA="${REPO_ROOT}/docs/contracts/intent_ir.schema.json"
export CAPABILITIES="${REPO_ROOT}/internal/governance/capabilities.json"

# ─────────────────────────────────────────
# 0. Arguments
# ─────────────────────────────────────────
export INTENT_FILE="${1:-}"
export WORK_DIR="${2:-$(pwd)}"
RESULT_ROOT_INPUT="${3:-${RAFAELIA_RESULT_ROOT:-${XDG_STATE_HOME:-${HOME:-$PWD}/.local/state}/rafaelia/vertical-slice/runs}}"

if [[ -z "${INTENT_FILE}" ]]; then
  echo "[ERROR] Usage: $0 <intent_ir.json> [working_directory] [result_root]" >&2
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

export WORK_DIR="$(cd "${WORK_DIR}" && pwd -P)"

echo "[INFO] intent_ir:    ${INTENT_FILE}"
echo "[INFO] working_dir:  ${WORK_DIR}"

# ─────────────────────────────────────────
# 1. Validate intent_ir against schema
# ─────────────────────────────────────────
echo "[STEP 1] Validating intent_ir schema..."

python3 -c "
import sys, json, os

intent_file = os.environ['INTENT_FILE']
schema_file = os.environ.get('INTENT_SCHEMA', '')

with open(intent_file, encoding='utf-8') as f:
    intent = json.load(f)

REQUIRED = ['schema', 'intent_id', 'action', 'target', 'inputs', 'constraints',
            'evidence_refs', 'requested_capabilities', 'risk', 'execution_gate']
missing = [key for key in REQUIRED if key not in intent]
if missing:
    print(f'[FAIL] Missing required fields: {missing}', file=sys.stderr)
    sys.exit(2)
if intent.get('schema') != 'rafaelia.intent.v1':
    print(f'[FAIL] schema must be rafaelia.intent.v1, got: {intent.get(\"schema\")!r}', file=sys.stderr)
    sys.exit(2)
if intent.get('risk') not in ('low', 'medium', 'high', 'critical'):
    print(f'[FAIL] invalid risk: {intent.get(\"risk\")!r}', file=sys.stderr)
    sys.exit(2)
if intent.get('execution_gate') not in ('allow', 'sandbox_only', 'human_review', 'blocked'):
    print(f'[FAIL] invalid execution_gate: {intent.get(\"execution_gate\")!r}', file=sys.stderr)
    sys.exit(2)

try:
    import jsonschema
    if os.path.exists(schema_file):
        with open(schema_file, encoding='utf-8') as sf:
            schema = json.load(sf)
        jsonschema.validate(intent, schema)
        print('[OK] Schema validated with jsonschema')
    else:
        print('[OK] Schema validated (minimal)')
except ImportError:
    print('[OK] Schema validated (jsonschema not installed, minimal checks passed)')
except Exception as exc:
    print(f'[FAIL] jsonschema: {exc}', file=sys.stderr)
    sys.exit(2)
"

# ─────────────────────────────────────────
# 2. Governance gate
# ─────────────────────────────────────────
echo "[STEP 2] Applying governance gate..."

GATE_RESULT="$(python3 -c "
import sys, json, os

intent_file = os.environ['INTENT_FILE']
caps_file = os.environ['CAPABILITIES']

with open(intent_file, encoding='utf-8') as f:
    intent = json.load(f)
with open(caps_file, encoding='utf-8') as f:
    caps_doc = json.load(f)

allowed_ids = {capability['id'] for capability in caps_doc['capabilities']}
V1_ALLOWED = {'git.read', 'git.diff'}

if intent.get('execution_gate') == 'blocked':
    print('BLOCKED:execution_gate=blocked'); sys.exit(0)
if intent.get('risk') == 'critical':
    print('BLOCKED:risk=critical'); sys.exit(0)
if intent.get('execution_gate') == 'human_review' or intent.get('risk') == 'high':
    print('HUMAN_REVIEW:manual approval required'); sys.exit(0)

requested = intent.get('requested_capabilities', [])
unknown = [capability for capability in requested if capability not in allowed_ids]
if unknown:
    print(f'BLOCKED:unknown capabilities {unknown}'); sys.exit(0)
non_v1 = [capability for capability in requested if capability not in V1_ALLOWED]
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
# 3. Build execution plan and immutable result directory
# ─────────────────────────────────────────
echo "[STEP 3] Building execution plan..."

export INTENT_ID="$(python3 -c "import json, os; print(json.load(open(os.environ['INTENT_FILE'], encoding='utf-8'))['intent_id'])")"
export STARTED_AT="$(date -u +%Y-%m-%dT%H:%M:%SZ)"
export PLAN_ID="plan-$(date -u +%Y%m%dT%H%M%SZ)-${INTENT_ID:0:8}"

mkdir -p -- "${RESULT_ROOT_INPUT}"
export RESULT_ROOT="$(cd "${RESULT_ROOT_INPUT}" && pwd -P)"
case "${RESULT_ROOT}" in
  "${WORK_DIR}"|"${WORK_DIR}"/*)
    echo "[ERROR] result_root must be outside working_directory to preserve read-only evidence." >&2
    exit 5
    ;;
esac

RUN_STAMP="$(date -u +%Y%m%dT%H%M%SZ)"
export RUN_DIR="$(mktemp -d "${RESULT_ROOT}/run-${RUN_STAMP}-XXXXXX")"
export RUN_ID="$(basename "${RUN_DIR}")"
export RESULT_PATH="${RUN_DIR}/execution_result.json"
COMMANDS_TSV="${RUN_DIR}/command_exit_codes.tsv"
printf 'name\texit_code\targs\n' > "${COMMANDS_TSV}"

echo "[INFO] plan_id=${PLAN_ID} intent_id=${INTENT_ID} run_id=${RUN_ID}"

sha256_file() {
  python3 - "$1" <<'PY'
import hashlib
import sys

path = sys.argv[1]
digest = hashlib.sha256()
with open(path, "rb") as handle:
    for block in iter(lambda: handle.read(1024 * 1024), b""):
        digest.update(block)
print(digest.hexdigest())
PY
}

export INTENT_SHA256="$(sha256_file "${INTENT_FILE}")"
export RUNNER_SCRIPT_SHA256="$(sha256_file "$0")"
export INTENT_SCHEMA_SHA256="$(sha256_file "${INTENT_SCHEMA}")"
export CAPABILITIES_SHA256="$(sha256_file "${CAPABILITIES}")"

if RUNNER_COMMIT="$(GIT_OPTIONAL_LOCKS=0 git -C "${REPO_ROOT}" rev-parse HEAD 2>/dev/null)"; then
  export RUNNER_COMMIT
else
  export RUNNER_COMMIT="TOKEN_VAZIO_RUNNER_COMMIT_UNAVAILABLE"
fi

run_git() {
  local name="$1"
  shift
  local stdout_path="${RUN_DIR}/${name}.stdout.log"
  local stderr_path="${RUN_DIR}/${name}.stderr.log"
  local exit_code

  set +e
  GIT_OPTIONAL_LOCKS=0 git -C "${WORK_DIR}" --no-pager "$@" >"${stdout_path}" 2>"${stderr_path}"
  exit_code=$?
  set -e

  printf '%s\t%s\t%s\n' "${name}" "${exit_code}" "$*" >> "${COMMANDS_TSV}"
  return "${exit_code}"
}

# ─────────────────────────────────────────
# 4. Execute the allowlisted, read-only Git probe
# ─────────────────────────────────────────
echo "[STEP 4] Capturing Git state with faithful exit codes..."

if run_git "git_is_inside_work_tree" rev-parse --is-inside-work-tree; then
  export GIT_INSIDE_RC=0
else
  export GIT_INSIDE_RC=$?
fi
if run_git "git_rev_parse_head" rev-parse HEAD; then
  export GIT_HEAD_RC=0
else
  export GIT_HEAD_RC=$?
fi
if run_git "git_status" status --short --branch; then
  export GIT_STATUS_RC=0
else
  export GIT_STATUS_RC=$?
fi
if run_git "git_diff_stat" diff --no-ext-diff --stat; then
  export GIT_DIFF_RC=0
else
  export GIT_DIFF_RC=$?
fi

if [[ "${GIT_HEAD_RC}" -eq 0 ]]; then
  export TARGET_COMMIT="$(tr -d '\r\n' < "${RUN_DIR}/git_rev_parse_head.stdout.log")"
else
  export TARGET_COMMIT="TOKEN_VAZIO_TARGET_COMMIT_UNAVAILABLE"
fi

export ENDED_AT="$(date -u +%Y-%m-%dT%H:%M:%SZ)"
export OVERALL_EXIT_CODE=0
export FINAL_STATE="success"
for command_exit in "${GIT_INSIDE_RC}" "${GIT_HEAD_RC}" "${GIT_STATUS_RC}" "${GIT_DIFF_RC}"; do
  if [[ "${command_exit}" -ne 0 ]]; then
    export OVERALL_EXIT_CODE="${command_exit}"
    export FINAL_STATE="failure"
    break
  fi
done

if [[ "${FINAL_STATE}" == "success" && "${RUNNER_COMMIT}" != TOKEN_VAZIO_* ]]; then
  export EVIDENCE_STATE="complete"
else
  export EVIDENCE_STATE="partial"
fi

# ─────────────────────────────────────────
# 5. Generate immutable execution_result.json and checksum manifest
# ─────────────────────────────────────────
echo "[STEP 5] Generating immutable execution_result.json..."

python3 - <<'PY'
import hashlib
import json
import os
from pathlib import Path

run_dir = Path(os.environ["RUN_DIR"])
commands_path = run_dir / "command_exit_codes.tsv"

command_args = {
    "git_is_inside_work_tree": ["rev-parse", "--is-inside-work-tree"],
    "git_rev_parse_head": ["rev-parse", "HEAD"],
    "git_status": ["status", "--short", "--branch"],
    "git_diff_stat": ["diff", "--no-ext-diff", "--stat"],
}

def sha256_bytes(value: bytes) -> str:
    return hashlib.sha256(value).hexdigest()

def read_bytes(path: Path) -> bytes:
    return path.read_bytes()

commands = []
for line in commands_path.read_text(encoding="utf-8").splitlines()[1:]:
    name, raw_exit_code, _ = line.split("\t", 2)
    stdout_path = run_dir / f"{name}.stdout.log"
    stderr_path = run_dir / f"{name}.stderr.log"
    commands.append(
        {
            "name": name,
            "args": command_args[name],
            "exit_code": int(raw_exit_code),
            "stdout_path": str(stdout_path),
            "stderr_path": str(stderr_path),
            "stdout_sha256": sha256_bytes(read_bytes(stdout_path)),
            "stderr_sha256": sha256_bytes(read_bytes(stderr_path)),
        }
    )

stdout_full = "".join(
    f"=== {command['name']} ===\n"
    + (run_dir / f"{command['name']}.stdout.log").read_text(encoding="utf-8", errors="replace")
    + "\n"
    for command in commands
)
stderr_full = "".join(
    f"=== {command['name']} ===\n"
    + (run_dir / f"{command['name']}.stderr.log").read_text(encoding="utf-8", errors="replace")
    + "\n"
    for command in commands
)

artifacts = []
for path in [commands_path] + [
    run_dir / f"{command['name']}.{stream}.log"
    for command in commands
    for stream in ("stdout", "stderr")
]:
    artifacts.append(
        {
            "name": path.name,
            "path": str(path),
            "sha256": sha256_bytes(read_bytes(path)),
        }
    )

intent = json.load(open(os.environ["INTENT_FILE"], encoding="utf-8"))
result = {
    "result_id": os.environ["RUN_ID"],
    "run_id": os.environ["RUN_ID"],
    "intent_id": os.environ["INTENT_ID"],
    "plan_id": os.environ["PLAN_ID"],
    "executed_command": "git",
    "args": [
        "rev-parse", "--is-inside-work-tree", "AND",
        "rev-parse", "HEAD", "AND",
        "status", "--short", "--branch", "AND",
        "diff", "--no-ext-diff", "--stat",
    ],
    "working_directory": os.environ["WORK_DIR"],
    "result_directory": str(run_dir),
    "started_at": os.environ["STARTED_AT"],
    "ended_at": os.environ["ENDED_AT"],
    "exit_code": int(os.environ["OVERALL_EXIT_CODE"]),
    "stdout_truncated": stdout_full[:4096],
    "stderr_truncated": stderr_full[:4096],
    "stdout_sha256": sha256_bytes(stdout_full.encode("utf-8")),
    "stderr_sha256": sha256_bytes(stderr_full.encode("utf-8")),
    "artifacts": artifacts,
    "command_results": commands,
    "final_state": os.environ["FINAL_STATE"],
    "evidence_state": os.environ["EVIDENCE_STATE"],
    "rollback_available": False,
    "source_chunk_refs": [reference["chunk_id"] for reference in intent.get("evidence_refs", [])],
    "target_commit": os.environ["TARGET_COMMIT"],
    "runner_commit": os.environ["RUNNER_COMMIT"].strip(),
    "input_hashes": {
        "intent_sha256": os.environ["INTENT_SHA256"],
        "intent_schema_sha256": os.environ["INTENT_SCHEMA_SHA256"],
        "capabilities_sha256": os.environ["CAPABILITIES_SHA256"],
        "runner_script_sha256": os.environ["RUNNER_SCRIPT_SHA256"],
    },
    "claim_allowed": False,
}
canonical = json.dumps(result, ensure_ascii=False, sort_keys=True, separators=(",", ":")).encode("utf-8")
result["receipt_payload_sha256"] = sha256_bytes(canonical)

result_path = Path(os.environ["RESULT_PATH"])
with result_path.open("x", encoding="utf-8") as handle:
    json.dump(result, handle, ensure_ascii=False, sort_keys=True, indent=2)
    handle.write("\n")
PY

python3 - "${RUN_DIR}" <<'PY'
import hashlib
import sys
from pathlib import Path

run_dir = Path(sys.argv[1])
manifest = run_dir / "SHA256SUMS"
rows = []
for path in sorted(run_dir.iterdir()):
    if path.is_file() and path.name != manifest.name:
        rows.append(f"{hashlib.sha256(path.read_bytes()).hexdigest()}  {path.name}\n")
with manifest.open("x", encoding="utf-8") as handle:
    handle.writelines(rows)
PY

echo "[DONE] Vertical slice v1 receipt emitted."
printf 'RUN_ID=%s\n' "${RUN_ID}"
printf 'RESULT_PATH=%s\n' "${RESULT_PATH}"
printf 'FINAL_STATE=%s\n' "${FINAL_STATE}"
exit "${OVERALL_EXIT_CODE}"
