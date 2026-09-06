from __future__ import annotations

import os
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
AUDITOR = ROOT / "scripts" / "rafcodephi_auditor.sh"


def test_report_markdown_literal_does_not_execute_backticks(tmp_path: Path) -> None:
    env = os.environ.copy()
    env["RAFCODEPHI_AUDITOR_OUT_DIR"] = str(tmp_path / "auditor")
    env["RAFCODEPHI_AUDITOR_RUN_ID"] = "pytest-literal-report"

    completed = subprocess.run(
        ["bash", str(AUDITOR), "report"],
        cwd=ROOT,
        env=env,
        text=True,
        capture_output=True,
        check=False,
    )

    assert completed.returncode == 0, completed.stderr
    assert "command not found" not in completed.stderr

    report = Path(env["RAFCODEPHI_AUDITOR_OUT_DIR"]) / "auditor-report.md"
    text = report.read_text(encoding="utf-8")
    assert "Métricas `skipped` indicam ausência de ferramenta" in text
