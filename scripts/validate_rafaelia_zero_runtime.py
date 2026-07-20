#!/usr/bin/env python3
import hashlib
import json
import pathlib
import subprocess
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]
HEADER = ROOT / "rafaelia/src/main/cpp/zero/include/rafz.h"
CORE = ROOT / "rafaelia/src/main/cpp/zero/rafz.c"
MODULE_MK = ROOT / "rafaelia/src/main/cpp/Android.mk"
APP_MK = ROOT / "app/src/main/cpp/Android.mk"
MODULE_JAVA = ROOT / "rafaelia/src/main/java/com/termux/rafaelia/RafaeliaZero.java"
APP_JAVA = ROOT / "app/src/main/java/com/termux/app/rafaelia/RafaeliaZeroRuntime.java"
APP_INIT = ROOT / "app/src/main/java/com/termux/app/TermuxApplication.java"
PROBE_CONTRACT = ROOT / "scripts/validate_rafaelia_zero_device_probe_contract.py"
EVIDENCE_CONTRACT = ROOT / "configs/rafaelia-zero-operational-evidence-contract.json"

EXPECTED_BLOBS = {
    HEADER: "019254937a4d7d50c3a862baa72966722faa38e2",
    CORE: "327f66261719c96e36afb668c180b1628b8c2669",
}
FORBIDDEN_CORE = (b"malloc(", b"calloc(", b"realloc(", b"free(", b"fopen(", b"printf(", b"syscall(")


def git_blob_sha(path: pathlib.Path) -> str:
    data = path.read_bytes()
    prefix = f"blob {len(data)}\0".encode("ascii")
    return hashlib.sha1(prefix + data).hexdigest()


def require_text(path: pathlib.Path, needles):
    text = path.read_text(encoding="utf-8")
    missing = [needle for needle in needles if needle not in text]
    return missing


def main() -> int:
    errors = []
    required_paths = (
        HEADER,
        CORE,
        MODULE_MK,
        APP_MK,
        MODULE_JAVA,
        APP_JAVA,
        APP_INIT,
        PROBE_CONTRACT,
        EVIDENCE_CONTRACT,
    )
    for path in required_paths:
        if not path.is_file():
            errors.append(f"missing:{path.relative_to(ROOT)}")

    for path, expected in EXPECTED_BLOBS.items():
        if path.is_file():
            actual = git_blob_sha(path)
            if actual != expected:
                errors.append(f"blob:{path.relative_to(ROOT)}:{actual}")

    if CORE.is_file():
        data = CORE.read_bytes()
        for token in FORBIDDEN_CORE:
            if token in data:
                errors.append(f"forbidden-core:{token.decode('ascii')}")
        if data.count(b'#include "rafz.h"') != 1:
            errors.append("core-include-contract")

    checks = {
        MODULE_MK: ["LOCAL_MODULE := termux_rafaelia_zero", "zero/rafz.c", "zero/rafz_android_jni.c"],
        APP_MK: ["LOCAL_MODULE := termux_rafaelia_zero_runtime", "../../../../rafaelia/src/main/cpp/zero/rafz.c"],
        MODULE_JAVA: ["System.loadLibrary(\"termux_rafaelia_zero\")", "payload.isDirect()"],
        APP_JAVA: ["System.loadLibrary(\"termux_rafaelia_zero_runtime\")", "payload.isDirect()"],
        APP_INIT: ["RafaeliaZeroRuntime.init()", "RafaeliaZeroRuntime.architectureId()"],
    }
    for path, needles in checks.items():
        if path.is_file():
            for needle in require_text(path, needles):
                errors.append(f"contract:{path.relative_to(ROOT)}:{needle}")

    probe_contract_status = "NOT_RUN"
    if PROBE_CONTRACT.is_file():
        completed = subprocess.run(
            [sys.executable, str(PROBE_CONTRACT)],
            cwd=str(ROOT),
            text=True,
            capture_output=True,
            check=False,
        )
        probe_contract_status = "PASS" if completed.returncode == 0 else "FAIL"
        if completed.returncode != 0:
            detail = (completed.stderr or completed.stdout).strip().replace("\n", " | ")
            errors.append(f"device-probe-contract:{detail}")

    evidence_state = {
        "arm32-legacy": "TOKEN_VAZIO",
        "arm64-modern": "TOKEN_VAZIO",
        "matrix": "TOKEN_VAZIO",
        "claim_allowed_device_matrix": False,
        "release_claim_allowed": False,
    }
    if EVIDENCE_CONTRACT.is_file():
        try:
            contract = json.loads(EVIDENCE_CONTRACT.read_text(encoding="utf-8"))
            current = contract.get("current_state", {})
            if contract.get("schema") != "rafaelia.zero.operational-evidence-contract.v1":
                errors.append("operational-evidence-contract:schema")
            expected_current = {
                "arm32-legacy": "TOKEN_VAZIO",
                "arm64-modern": "TOKEN_VAZIO",
                "matrix": "TOKEN_VAZIO",
                "claim_allowed_device_matrix": False,
            }
            for key, expected in expected_current.items():
                if current.get(key) != expected:
                    errors.append(f"operational-evidence-contract:{key}")
        except (OSError, json.JSONDecodeError) as exc:
            errors.append(f"operational-evidence-contract:{exc}")

    result = {
        "schema": "rafaelia.zero.android-runtime-validation.v3",
        "status": "PASS" if not errors else "FAIL",
        "errors": errors,
        "claim_allowed_device_execution": False,
        "device_probe_and_evidence_contract": probe_contract_status,
        "device_evidence_state": evidence_state,
        "static_pass_promotes_device_claim": False,
        "core_git_blobs": {str(path.relative_to(ROOT)): sha for path, sha in EXPECTED_BLOBS.items()},
    }
    print(json.dumps(result, sort_keys=True))
    return 0 if not errors else 1


if __name__ == "__main__":
    raise SystemExit(main())
