from __future__ import annotations

import importlib.util
import json
import zipfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
VALIDATOR_PATH = ROOT / "scripts" / "validate_real_arm_bootstrap_core.py"

spec = importlib.util.spec_from_file_location("validate_real_arm_bootstrap_core", VALIDATOR_PATH)
assert spec is not None and spec.loader is not None
validator = importlib.util.module_from_spec(spec)
spec.loader.exec_module(validator)

READY_TOKENS = (
    "BOOTSTRAP_REAL_APT_READY=1",
    "BOOTSTRAP_REAL_DPKG_READY=1",
    "BOOTSTRAP_REAL_PROOT_READY=1",
    "BOOTSTRAP_REAL_COREUTILS_READY=1",
    "BOOTSTRAP_CA_CERTIFICATES_READY=1",
    "BOOTSTRAP_DNS_RESOLVER_READY=1",
    "BOOTSTRAP_MINIMUM_COMMANDS_READY=1",
)


def write_zip(path: Path, *, legacy_binary: bool = False, omit: str | None = None) -> None:
    canonical = validator.PREFIX
    entries: dict[str, bytes] = {
        "bin/sh": b"#!/system/bin/sh\n",
        "bin/bash": b"bash-placeholder\n",
        "bin/apt": b"apt-placeholder\n",
        "bin/apt-get": b"apt-get-placeholder\n",
        "bin/dpkg": b"dpkg-placeholder\n",
        "bin/pkg": f"#!/system/bin/sh\nPREFIX={canonical}\n".encode(),
        "bin/proot": f"#!/system/bin/sh\nPREFIX={canonical}\n".encode(),
        "bin/proot.real": b"proot-real-placeholder\n",
        "bin/cat": f"#!/system/bin/sh\nPREFIX={canonical}\n".encode(),
        "bin/ls": f"#!/system/bin/sh\nPREFIX={canonical}\n".encode(),
        "bin/clear": f"#!/system/bin/sh\nPREFIX={canonical}\n".encode(),
        "bin/grep": f"#!/system/bin/sh\nPREFIX={canonical}\n".encode(),
        "etc/apt/sources.list": b"deb https://packages.termux.dev/apt/termux-main stable main\n",
        "etc/resolv.conf": b"nameserver 1.1.1.1\n",
        "etc/rafcodephi-core.env": f"TERMUX_PREFIX={canonical}\n".encode(),
        "BOOTSTRAP_INFO": ("\n".join(READY_TOKENS) + "\n").encode(),
        "SYMLINKS.txt": b"",
    }
    if legacy_binary:
        entries["bin/apt"] = b"\x7fELF\x00prefix=/data/data/com.termux/files/usr\x00"
    if omit is not None:
        entries.pop(omit)
    with zipfile.ZipFile(path, "w", compression=zipfile.ZIP_DEFLATED) as zf:
        for name, data in entries.items():
            zf.writestr(name, data)


def test_runtime_audit_distinguishes_pass_blocked_and_fail(tmp_path: Path) -> None:
    passing = tmp_path / "pass.zip"
    blocked = tmp_path / "blocked.zip"
    failing = tmp_path / "fail.zip"
    write_zip(passing)
    write_zip(blocked, legacy_binary=True)
    write_zip(failing, omit="bin/dpkg")

    pass_report = validator.audit(passing)
    blocked_report = validator.audit(blocked)
    fail_report = validator.audit(failing)

    assert pass_report["state"] == "PASS"
    assert pass_report["claim_allowed_structural_real_pkg"] is True
    assert blocked_report["state"] == "BLOCKED"
    assert blocked_report["reason"] == "UPSTREAM_BINARY_PREFIX_REBUILD_REQUIRED"
    assert blocked_report["binary_risk_entry_count"] == 1
    assert blocked_report["claim_allowed_structural_real_pkg"] is False
    assert blocked_report["release_allowed"] is False
    assert fail_report["state"] == "FAIL"
    assert fail_report["reason"] == "STRUCTURAL_BOOTSTRAP_CONTRACT_FAILURE"
    assert fail_report["release_allowed"] is False


def test_matrix_preserves_blocked_and_fail_dominates(tmp_path: Path) -> None:
    passing = tmp_path / "pass.zip"
    blocked = tmp_path / "blocked.zip"
    failing = tmp_path / "fail.zip"
    write_zip(passing)
    write_zip(blocked, legacy_binary=True)
    write_zip(failing, omit="bin/dpkg")

    pass_report = validator.audit(passing)
    blocked_report = validator.audit(blocked)
    fail_report = validator.audit(failing)

    state, reason = validator.classify_matrix_state([pass_report, blocked_report])
    assert state == "BLOCKED"
    assert reason == "AT_LEAST_ONE_ARTIFACT_REQUIRES_PREFIX_REBUILD"

    state, reason = validator.classify_matrix_state([blocked_report, fail_report])
    assert state == "FAIL"
    assert reason == "AT_LEAST_ONE_ARTIFACT_FAILED_AUDIT"


def test_cli_matrix_receipt_keeps_blocked_distinct_from_fail(tmp_path: Path) -> None:
    passing = tmp_path / "pass.zip"
    blocked = tmp_path / "blocked.zip"
    receipt = tmp_path / "matrix.json"
    write_zip(passing)
    write_zip(blocked, legacy_binary=True)

    rc = validator.main(["--json", str(receipt), str(passing), str(blocked)])
    payload = json.loads(receipt.read_text(encoding="utf-8"))

    assert rc == 1  # promotion remains fail-closed for BLOCKED
    assert payload["schema"] == "rafcodephi-real-pkg-prefix-audit-matrix/v2"
    assert payload["state"] == "BLOCKED"
    assert payload["reason"] == "AT_LEAST_ONE_ARTIFACT_REQUIRES_PREFIX_REBUILD"
    assert payload["claim_allowed_structural_real_pkg"] is False
    assert payload["claim_allowed_device_runtime"] is False
    assert payload["claim_allowed_pkg_runtime"] is False
    assert payload["release_allowed"] is False
