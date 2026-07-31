#!/usr/bin/env bash
# run_ham_readonly_flow.sh — HAM v1 governed wrapper for Vertical Slice v1.
#
# Usage:
#   bash scripts/vertical_slice/run_ham_readonly_flow.sh \
#     <ham_request.json> <intent_ir.json> [working_directory] [result_root]
#
# The wrapper adds no executable capability. It validates the human/AI request,
# pins the local repository adapter, invokes the existing read-only vertical
# slice and emits an immutable HAM receipt beside the base-flow receipt.
set -euo pipefail
umask 077

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd -P)"
HAM_FILE="${1:-}"
INTENT_FILE="${2:-}"
WORK_DIR="${3:-$(pwd)}"
RESULT_ROOT_INPUT="${4:-${RAFAELIA_RESULT_ROOT:-${XDG_STATE_HOME:-${HOME:-$PWD}/.local/state}/rafaelia/vertical-slice/runs}}"
ADAPTER_FILE="${REPO_ROOT}/contracts/rafaelia-human-ai-adapter.v1.json"
BASE_FLOW="${SCRIPT_DIR}/run_readonly_flow.sh"

if [[ -z "${HAM_FILE}" || -z "${INTENT_FILE}" ]]; then
  echo "[ERROR] Usage: $0 <ham_request.json> <intent_ir.json> [working_directory] [result_root]" >&2
  exit 1
fi
for required in "${HAM_FILE}" "${INTENT_FILE}" "${ADAPTER_FILE}" "${BASE_FLOW}"; do
  if [[ ! -f "${required}" ]]; then
    echo "[ERROR] Required file not found: ${required}" >&2
    exit 1
  fi
done
if [[ ! -d "${WORK_DIR}" ]]; then
  echo "[ERROR] Working directory not found: ${WORK_DIR}" >&2
  exit 1
fi

HAM_DECISION_FILE="$(mktemp "${TMPDIR:-/tmp}/raf-ham-decision.XXXXXX.json")"
BASE_LOG="$(mktemp "${TMPDIR:-/tmp}/raf-ham-base.XXXXXX.log")"
trap 'rm -f "${HAM_DECISION_FILE}" "${BASE_LOG}"' EXIT
export HAM_FILE ADAPTER_FILE HAM_DECISION_FILE

python3 - <<'PY'
import hashlib
import json
import os
import sys

request = json.load(open(os.environ["HAM_FILE"], encoding="utf-8"))
adapter = json.load(open(os.environ["ADAPTER_FILE"], encoding="utf-8"))

errors = []

def require(condition, code, message):
    if not condition:
        errors.append({"code": code, "message": message})

require(request.get("schema") == "raf.human-ai.middleware.v1", "HAM_SCHEMA", "invalid HAM schema")
require(adapter.get("schema") == "raf.human-ai.adapter.v1", "ADAPTER_SCHEMA", "invalid adapter schema")
require(adapter.get("repository") == "rafaelmeloreisnovo/termux-app-rafacodephi", "ADAPTER_REPO", "adapter repository mismatch")
require(adapter.get("contract", {}).get("commit_pin") == "fd184197f72b1669102fc76be771d52e02fc3902", "ADAPTER_PIN", "canonical contract pin mismatch")

execution = request.get("execution", {})
ai_lane = request.get("ai_lane", {})
human_lane = request.get("human_lane", {})
people = request.get("people", {})
risk = request.get("risk", {})
friction = request.get("friction", {})
data_boundary = request.get("data_boundary", {})

require(execution.get("target_repository") == adapter.get("repository"), "TARGET", "request targets another repository")
require(execution.get("effect_class") == "READ_ONLY", "EFFECT", "v1 wrapper accepts READ_ONLY only")
require("READ_ONLY" in adapter.get("allowed_effects", []), "ALLOWLIST", "adapter does not allow READ_ONLY")
require("READ_ONLY" not in adapter.get("forbidden_effects", []), "FORBIDDEN", "READ_ONLY is forbidden by adapter")
require(ai_lane.get("may_execute") is False, "AI_EXECUTE", "AI may not execute")
require(ai_lane.get("may_finalize") is False, "AI_FINALIZE", "AI may not finalize")
require(ai_lane.get("may_expand_scope") is False, "AI_SCOPE", "AI may not expand scope")
require(people.get("human_final_decision") is True, "HUMAN_FINAL", "human final decision required")
require(human_lane.get("decision") in {"APPROVE_BOUNDED", "APPROVE_TWO_STEP"}, "HUMAN_DECISION", "bounded human approval required")
require(human_lane.get("consent_state") in {"APPROVED", "NOT_REQUIRED"}, "CONSENT", "consent must be approved or not required")
require(risk.get("level") != "CRITICAL", "CRITICAL", "critical risk is blocked")
require(friction.get("stop_on_no_new_evidence") is True, "LOOP_STOP", "no-evidence stop must be enabled")
require(isinstance(friction.get("loop_budget"), int) and isinstance(friction.get("current_loop"), int) and friction["current_loop"] <= friction["loop_budget"], "LOOP_BUDGET", "loop budget exceeded")
require(data_boundary.get("destination_visibility") != "PUBLIC", "PUBLIC_OUTPUT", "read-only v1 does not publish")
require(data_boundary.get("raw_data_export") is False, "RAW_EXPORT", "raw export must be disabled")

secret_keys = {
    "access_token", "refresh_token", "password", "secret", "api_key",
    "private_key", "authorization_header", "cookie", "session_token",
}

def walk(value, path="$"):
    if isinstance(value, dict):
        for key, child in value.items():
            yield path + "." + key, key, child
            yield from walk(child, path + "." + key)
    elif isinstance(value, list):
        for index, child in enumerate(value):
            yield from walk(child, f"{path}[{index}]")

for path, key, value in walk(request):
    if key.lower() in secret_keys and value not in (None, "", False):
        errors.append({"code": "SECRET", "message": f"secret material at {path}"})

canonical = json.dumps(request, ensure_ascii=False, sort_keys=True, separators=(",", ":")).encode("utf-8")
decision = {
    "schema": "raf.human-ai.gate-decision.v1",
    "request_id": request.get("request_id"),
    "request_sha256": hashlib.sha256(canonical).hexdigest(),
    "adapter_repository": adapter.get("repository"),
    "adapter_contract_pin": adapter.get("contract", {}).get("commit_pin"),
    "effect_class": execution.get("effect_class"),
    "consent_state": human_lane.get("consent_state"),
    "human_decision": human_lane.get("decision"),
    "errors": errors,
    "decision": "ALLOW_BOUNDED" if not errors else "BLOCKED",
    "claim_allowed": False,
}
json.dump(decision, open(os.environ["HAM_DECISION_FILE"], "w", encoding="utf-8"), ensure_ascii=False, sort_keys=True, indent=2)
print(f"[HAM] {decision['decision']} request={decision.get('request_id')} sha256={decision['request_sha256']}")
if errors:
    for error in errors:
        print(f"[HAM][BLOCK] {error['code']}: {error['message']}", file=sys.stderr)
    sys.exit(5)
PY

# The base flow remains the sole command executor and its allowlist is
# unchanged in effect: Git metadata/status/diff reads only.
set +e
bash "${BASE_FLOW}" "${INTENT_FILE}" "${WORK_DIR}" "${RESULT_ROOT_INPUT}" >"${BASE_LOG}" 2>&1
BASE_EXIT=$?
set -e
cat "${BASE_LOG}"

RESULT_PATH="$(awk -F= '$1 == "RESULT_PATH" {path=$2} END {print path}' "${BASE_LOG}")"
if [[ -z "${RESULT_PATH}" || ! -f "${RESULT_PATH}" ]]; then
  echo "[ERROR] Base flow did not emit an execution result path." >&2
  exit 6
fi

mkdir -p -- "${RESULT_ROOT_INPUT}"
RESULT_ROOT="$(cd "${RESULT_ROOT_INPUT}" && pwd -P)"
case "${RESULT_PATH}" in
  "${RESULT_ROOT}"/*) ;;
  *)
    echo "[ERROR] Base flow result escaped the declared result root." >&2
    exit 7
    ;;
esac

export INTENT_FILE WORK_DIR RESULT_PATH BASE_EXIT
python3 - <<'PY'
import hashlib
import json
import os
from pathlib import Path

request = json.load(open(os.environ["HAM_FILE"], encoding="utf-8"))
decision = json.load(open(os.environ["HAM_DECISION_FILE"], encoding="utf-8"))
result_path = Path(os.environ["RESULT_PATH"])
result_bytes = result_path.read_bytes()
result = json.loads(result_bytes)
intent = json.load(open(os.environ["INTENT_FILE"], encoding="utf-8"))
base_exit_code = int(os.environ["BASE_EXIT"])
base_success = base_exit_code == 0 and result.get("final_state") == "success"

receipt = {
    "schema": "raf.human-ai.execution-receipt.v1",
    "request_id": decision["request_id"],
    "request_sha256": decision["request_sha256"],
    "adapter_repository": decision["adapter_repository"],
    "adapter_contract_pin": decision["adapter_contract_pin"],
    "gate_decision": decision["decision"],
    "consent_state": decision["consent_state"],
    "human_decision": decision["human_decision"],
    "intent_id": intent.get("intent_id"),
    "effect_class": decision["effect_class"],
    "working_directory_logical": ".",
    "execution_result_path": str(result_path),
    "execution_result_sha256": hashlib.sha256(result_bytes).hexdigest(),
    "execution_result": result,
    "base_exit_code": base_exit_code,
    "final_state": "success" if base_success else "failure",
    "source_preserved": True,
    "claim_allowed": False,
    "F_ok": ["HAM gate allowed bounded read"] + (
        ["base read-only flow recorded success with faithful exit codes"] if base_success else []
    ),
    "F_gap": [] if base_success else [
        f"base read-only flow exit={base_exit_code}, state={result.get('final_state')}"
    ],
    "F_next": ["human review of receipt before any broader capability"],
}
canonical = json.dumps(receipt, ensure_ascii=False, sort_keys=True, separators=(",", ":")).encode("utf-8")
receipt["receipt_sha256"] = hashlib.sha256(canonical).hexdigest()
receipt_path = result_path.with_name("ham_execution_receipt.json")
with receipt_path.open("x", encoding="utf-8") as handle:
    json.dump(receipt, handle, ensure_ascii=False, sort_keys=True, indent=2)
    handle.write("\n")
print(f"[HAM][OK] HAM_RECEIPT_PATH={receipt_path} sha256={receipt['receipt_sha256']}")
PY

exit "${BASE_EXIT}"
