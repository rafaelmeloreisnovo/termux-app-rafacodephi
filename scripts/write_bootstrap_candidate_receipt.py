#!/usr/bin/env python3
"""Write a fail-closed receipt for a structurally validated bootstrap APK candidate.

This receipt is build evidence only. It never promotes device/runtime/pkg claims.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
from datetime import datetime, timezone
from pathlib import Path

SCHEMA = "rafcodephi-bootstrap-candidate-receipt/v1"


def sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as fh:
        for chunk in iter(lambda: fh.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()


def fail(message: str) -> None:
    raise SystemExit(f"bootstrap candidate receipt: {message}")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--matrix", type=Path, required=True)
    parser.add_argument("--apk", type=Path, required=True)
    parser.add_argument("--embedded-bootstrap", type=Path, required=True)
    parser.add_argument("--target-arch", required=True, choices=("arm", "aarch64"))
    parser.add_argument("--out", type=Path, required=True)
    args = parser.parse_args()

    for path in (args.matrix, args.apk, args.embedded_bootstrap):
        if not path.is_file() or path.stat().st_size <= 0:
            fail(f"required artifact missing or empty: {path}")

    matrix = json.loads(args.matrix.read_text(encoding="utf-8"))
    if matrix.get("schema") != "rafcodephi-bootstrap-profile-matrix/v2":
        fail(f"unsupported matrix schema: {matrix.get('schema')}")
    if matrix.get("structural_state") != "PASS":
        fail("matrix structural_state is not PASS")
    profile_by_arch = matrix.get("embedded_profile_by_arch") or {}
    if profile_by_arch.get(args.target_arch) != "real-pkg":
        fail(f"target arch {args.target_arch} is not real-pkg in matrix")
    if matrix.get("claim_allowed") is not False or matrix.get("release_allowed") is not False:
        fail("matrix claim boundary is unexpectedly open")

    receipt = {
        "schema": SCHEMA,
        "generated_at_utc": datetime.now(timezone.utc).isoformat().replace("+00:00", "Z"),
        "git_sha": os.environ.get("GITHUB_SHA", "UNAVAILABLE"),
        "package_name": "com.termux.rafacodephi",
        "target_arch": args.target_arch,
        "requested_profile": matrix.get("requested_profile"),
        "real_pkg_arch_request": matrix.get("real_pkg_arch_request"),
        "embedded_profile_by_arch": profile_by_arch,
        "matrix_path": str(args.matrix),
        "matrix_sha256": sha256(args.matrix),
        "embedded_bootstrap_path": str(args.embedded_bootstrap),
        "embedded_bootstrap_sha256": sha256(args.embedded_bootstrap),
        "apk_path": str(args.apk),
        "apk_sha256": sha256(args.apk),
        "structural_state": "PASS",
        "device_validation": "TOKEN_VAZIO",
        "runtime_pkg_update": "NOT_MEASURED",
        "runtime_pkg_install_smoke": "NOT_MEASURED",
        "dns_tls_repository": "NOT_MEASURED",
        "claim_allowed_structural_candidate": True,
        "claim_allowed_device_runtime": False,
        "claim_allowed_real_pkg_runtime": False,
        "release_allowed": False,
        "token_vazio": matrix.get("token_vazio", []),
        "claim_boundary": (
            "This proves only that the exact embedded real-pkg bootstrap and APK candidate were "
            "materialized and structurally validated in CI. Physical install, pkg update, pkg install, "
            "DNS/TLS repository access and release suitability remain unmeasured."
        ),
    }
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_text(json.dumps(receipt, sort_keys=True, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(receipt, sort_keys=True, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
