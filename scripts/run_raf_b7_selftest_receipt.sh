#!/usr/bin/env sh
# Compile and execute the smallest B7 selftest, then preserve a hash-anchored
# append-only receipt. No network, package installation, APK installation or
# global claim promotion is performed.
set -eu

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
OUT_DIR=${1:-"$ROOT/runtime-receipts"}
RUNNER="$ROOT/scripts/run_raf_b7_selftest.sh"
CC_BIN=${CC:-clang}

for command in sh python3 sha256sum uname date; do
    if ! command -v "$command" >/dev/null 2>&1; then
        echo "[FALHA] comando obrigatório ausente: $command" >&2
        exit 2
    fi
done
if ! command -v "$CC_BIN" >/dev/null 2>&1; then
    echo "[FALHA] compilador ausente: $CC_BIN" >&2
    exit 2
fi
if [ ! -x "$RUNNER" ]; then
    echo "[FALHA] selftest ausente ou não executável: $RUNNER" >&2
    exit 2
fi

mkdir -p "$OUT_DIR"
STAMP=$(date -u +%Y%m%dT%H%M%SZ)
EVENT_ID="RAF-B7-SELFTEST-$STAMP-$$"
STDOUT_FILE="$OUT_DIR/${EVENT_ID}.stdout"
STDERR_FILE="$OUT_DIR/${EVENT_ID}.stderr"
RECEIPT="$OUT_DIR/${EVENT_ID}.json"
BINARY=${TMPDIR:-/tmp}/raf_b7_orchestrator_selftest

set +e
CC="$CC_BIN" sh "$RUNNER" >"$STDOUT_FILE" 2>"$STDERR_FILE"
RUN_EXIT=$?
set -e

if git -C "$ROOT" rev-parse --verify HEAD >/dev/null 2>&1; then
    REPOSITORY_COMMIT=$(git -C "$ROOT" rev-parse HEAD)
else
    REPOSITORY_COMMIT=TOKEN_VAZIO
fi

DEVICE_CLASS=NON_ANDROID_LOCAL
ANDROID_SDK=
CPU_ABI=
if command -v getprop >/dev/null 2>&1; then
    ANDROID_SDK=$(getprop ro.build.version.sdk 2>/dev/null || true)
    CPU_ABI=$(getprop ro.product.cpu.abi 2>/dev/null || true)
    if [ -n "$ANDROID_SDK" ] || [ -n "$CPU_ABI" ]; then
        DEVICE_CLASS=ANDROID_RUNTIME
    fi
fi
case ${PREFIX:-} in
    *com.termux.rafacodephi*|*com.termux*) DEVICE_CLASS=TERMUX_ANDROID ;;
esac
if [ -z "$CPU_ABI" ]; then
    CPU_ABI=$(uname -m)
fi

set +e
python3 - \
    "$RECEIPT" \
    "$EVENT_ID" \
    "$STAMP" \
    "$ROOT" \
    "$REPOSITORY_COMMIT" \
    "$CC_BIN" \
    "$RUN_EXIT" \
    "$STDOUT_FILE" \
    "$STDERR_FILE" \
    "$BINARY" \
    "$DEVICE_CLASS" \
    "$CPU_ABI" \
    "$ANDROID_SDK" \
    "$(uname -s)" \
    "$(uname -r)" \
    "${PREFIX:-TOKEN_VAZIO}" <<'PY'
from __future__ import annotations

import hashlib
import json
import re
import subprocess
import sys
from pathlib import Path

(
    receipt_path,
    event_id,
    stamp,
    root_value,
    repository_commit,
    cc_bin,
    run_exit_value,
    stdout_value,
    stderr_value,
    binary_value,
    device_class,
    cpu_abi,
    android_sdk,
    kernel_name,
    kernel_release,
    prefix,
) = sys.argv[1:]

root = Path(root_value)
stdout_path = Path(stdout_value)
stderr_path = Path(stderr_value)
binary_path = Path(binary_value)
run_exit = int(run_exit_value)


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for block in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


source_paths = [
    root / "rmr/Rrr/raf_b7_orchestrator.h",
    root / "rmr/Rrr/raf_b7_orchestrator.c",
    root / "tools/raf_b7_orchestrator_selftest.c",
    root / "scripts/run_raf_b7_selftest.sh",
]
inputs = []
missing_inputs = []
for path in source_paths:
    if path.is_file():
        inputs.append(
            {
                "path": path.relative_to(root).as_posix(),
                "sha256": sha256(path),
                "size": path.stat().st_size,
            }
        )
    else:
        missing_inputs.append(path.relative_to(root).as_posix())

stdout_text = stdout_path.read_text(encoding="utf-8", errors="replace")
stderr_text = stderr_path.read_text(encoding="utf-8", errors="replace")
lines = [line.strip() for line in stdout_text.splitlines() if line.strip()]
pass_line = next((line for line in reversed(lines) if line.startswith("PASS ")), "")
match = re.fullmatch(
    r"PASS bytes=(\d+) crc32c=([0-9a-fA-F]{8}) caps=([0-9a-fA-F]{8}) receipts=(\d+)",
    pass_line,
)
output_contract = run_exit == 0 and match is not None and not missing_inputs

try:
    compiler_version = subprocess.run(
        [cc_bin, "--version"],
        check=False,
        capture_output=True,
        text=True,
        timeout=10,
    ).stdout.splitlines()[0]
except (OSError, subprocess.TimeoutExpired, IndexError):
    compiler_version = "TOKEN_VAZIO"

binary = {
    "path": str(binary_path),
    "present": binary_path.is_file(),
    "sha256": sha256(binary_path) if binary_path.is_file() else "TOKEN_VAZIO",
    "size": binary_path.stat().st_size if binary_path.is_file() else 0,
}
parsed = {
    "bytes": int(match.group(1)) if match else None,
    "crc32c": match.group(2).lower() if match else None,
    "capabilities": match.group(3).lower() if match else None,
    "receipt_count": int(match.group(4)) if match else None,
}

receipt = {
    "schema": "rafaelia.b7-device-selftest-receipt.v1",
    "event_id": event_id,
    "recorded_at_compact_utc": stamp,
    "repository": "rafaelmeloreisnovo/termux-app-rafacodephi",
    "repository_commit": repository_commit,
    "inputs": inputs,
    "missing_inputs": missing_inputs,
    "execution_context": {
        "device_class": device_class,
        "cpu_abi": cpu_abi or "TOKEN_VAZIO",
        "android_sdk": android_sdk or "TOKEN_VAZIO",
        "kernel_name": kernel_name,
        "kernel_release": kernel_release,
        "prefix": prefix,
        "compiler": cc_bin,
        "compiler_version": compiler_version,
    },
    "execution": {
        "command": "CC=<compiler> sh scripts/run_raf_b7_selftest.sh",
        "exit_code": run_exit,
        "stdout_path": stdout_path.name,
        "stdout_sha256": sha256(stdout_path),
        "stderr_path": stderr_path.name,
        "stderr_sha256": sha256(stderr_path),
        "pass_line": pass_line or "TOKEN_VAZIO",
        "parsed": parsed,
        "binary": binary,
    },
    "checks": {
        "all_inputs_present": "PASS" if not missing_inputs else "FAIL",
        "compile_and_execute": "PASS" if run_exit == 0 else "FAIL",
        "output_contract": "PASS" if output_contract else "FAIL",
        "source_hashes": "PASS" if not missing_inputs else "FAIL",
        "binary_hash": "PASS" if binary_path.is_file() else "TOKEN_VAZIO",
        "android_physical_device": (
            "PASS_CONTEXT_ONLY" if device_class == "TERMUX_ANDROID" else "TOKEN_VAZIO"
        ),
    },
    "selftest_internal_attestation_exercised": bool(output_contract),
    "claim_allowed": False,
    "F_ok": (
        "B7 selftest compiled, executed and emitted the exact PASS contract with hashed sources, binary and output"
        if output_contract
        else "receipt preserved the failed or incomplete selftest attempt"
    ),
    "F_gap": (
        "global runtime, APK installation and cross-device replication remain unproved"
        if output_contract
        else "selftest compile/execute or output contract did not close"
    ),
    "F_next": (
        "repeat this exact receipt on the second Android ABI and compare source, binary, output and CRC32C hashes"
        if output_contract
        else "inspect stderr and repair the first failing compile, execution or output-contract gate"
    ),
}

Path(receipt_path).write_text(
    json.dumps(receipt, ensure_ascii=False, indent=2, sort_keys=True) + "\n",
    encoding="utf-8",
)
raise SystemExit(0 if output_contract else 4)
PY
RECEIPT_STATUS=$?
set -e

RECEIPT_SHA256=$(sha256sum "$RECEIPT" | awk '{print $1}')
printf '%s  %s\n' "$RECEIPT_SHA256" "$(basename "$RECEIPT")" >"$RECEIPT.sha256"
(
    cd "$(dirname "$RECEIPT")"
    sha256sum -c "$(basename "$RECEIPT.sha256")"
)

cat "$STDOUT_FILE"
if [ -s "$STDERR_FILE" ]; then
    cat "$STDERR_FILE" >&2
fi
echo "[RAF] b7_event=$EVENT_ID"
echo "[RAF] b7_receipt=$RECEIPT"
echo "[RAF] b7_receipt_sha256=$RECEIPT_SHA256"

if [ "$RUN_EXIT" -ne 0 ]; then
    exit "$RUN_EXIT"
fi
exit "$RECEIPT_STATUS"
