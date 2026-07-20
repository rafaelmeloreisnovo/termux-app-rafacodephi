#!/usr/bin/env python3
"""Verify that the canonical browser materialization cannot downgrade HTTPS."""

from __future__ import annotations

import json
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
MATERIALIZER = ROOT / "scripts/materialize_browser_fail_closed.py"
MATRIX = ROOT / "configs/operational-technical-coherence.json"

FORBIDDEN = (
    "ctx->port=80u;ctx->use_tls=0;",
    "[FALLBACK] Usando HTTP para demo",
    "crypto não implementado — usando HTTP para demo",
)

REQUIRED = (
    "HTTPS nunca pode ser rebaixado para HTTP plaintext",
    "ctx->tls=TLS_ERROR;",
    "HTTPS bloqueado: TLS criptográfico ainda não implementado",
    "return-2;",
)


def main() -> int:
    checks: list[dict[str, object]] = []
    failures: list[str] = []

    def check(name: str, condition: bool, detail: object) -> None:
        checks.append({"name": name, "status": "PASS" if condition else "FAIL", "detail": detail})
        if not condition:
            failures.append(name)

    with tempfile.TemporaryDirectory(prefix="raf-browser-fail-closed-") as tmp:
        output = Path(tmp) / "browser-build-safe.sh"
        completed = subprocess.run(
            [sys.executable, str(MATERIALIZER), "--output", str(output)],
            cwd=ROOT,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            check=False,
        )
        check("materializer_exit_zero", completed.returncode == 0, completed.stdout + completed.stderr)

        materialized = output.read_text(encoding="utf-8") if output.is_file() else ""
        check(
            "plaintext_downgrade_absent",
            all(token not in materialized for token in FORBIDDEN),
            [token for token in FORBIDDEN if token in materialized],
        )
        check(
            "fail_closed_markers_present",
            all(token in materialized for token in REQUIRED),
            [token for token in REQUIRED if token not in materialized],
        )
        check(
            "materialized_script_executable",
            output.is_file() and bool(output.stat().st_mode & 0o111),
            oct(output.stat().st_mode) if output.is_file() else "missing",
        )

    try:
        matrix = json.loads(MATRIX.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        matrix = {}
        failures.append("matrix_readable")
        checks.append({"name": "matrix_readable", "status": "FAIL", "detail": str(exc)})

    components = {item.get("id"): item for item in matrix.get("components", [])}
    gate = components.get("browser.https.failclosed.materializer", {})
    tls = components.get("browser.tls13", {})

    check(
        "matrix_records_fail_closed_gate",
        gate.get("state") == "VERIFIED_HOST"
        and gate.get("claim_allowed") is True
        and gate.get("path") == "scripts/materialize_browser_fail_closed.py",
        gate,
    )
    check(
        "tls_claim_remains_blocked",
        tls.get("claim_allowed") is False and tls.get("state") == "PROTOTYPE_FAIL_CLOSED_REQUIRED",
        tls,
    )

    report = {
        "schema": "raf.browser.fail-closed-gate.v1",
        "status": "PASS" if not failures else "FAIL",
        "claim_allowed_tls": False,
        "https_policy": "FAIL_CLOSED",
        "checks": checks,
        "failures": failures,
    }
    json.dump(report, sys.stdout, ensure_ascii=False, indent=2)
    sys.stdout.write("\n")
    return 0 if not failures else 1


if __name__ == "__main__":
    raise SystemExit(main())
