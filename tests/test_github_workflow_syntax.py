#!/usr/bin/env python3
"""Fail closed when a GitHub Actions workflow is malformed or has invalid Bash."""

from __future__ import annotations

from pathlib import Path
import re
import subprocess
import sys

import yaml


ROOT = Path(__file__).resolve().parents[1]
WORKFLOWS = ROOT / ".github" / "workflows"
GITHUB_EXPRESSION = re.compile(r"\$\{\{.*?\}\}", re.DOTALL)


def blocked(message: str) -> None:
    raise SystemExit(f"RAFCODEPHI_WORKFLOW_SYNTAX=BLOCKED {message}")


def validate_bash(path: Path, job_name: str, step_index: int, step: dict, defaults: dict) -> bool:
    script = step.get("run")
    if not isinstance(script, str):
        return False

    run_defaults = defaults.get("run", {}) if isinstance(defaults, dict) else {}
    shell = step.get("shell", run_defaults.get("shell", "bash"))
    if not str(shell).startswith(("bash", "sh")):
        return True

    result = subprocess.run(
        ["bash", "-n"],
        input=GITHUB_EXPRESSION.sub("CI_EXPRESSION", script),
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        check=False,
    )
    if result.returncode:
        blocked(
            f"bash_parse path={path.relative_to(ROOT)} job={job_name} step={step_index} "
            f"name={step.get('name', 'unnamed')}: {result.stderr.strip()}"
        )
    return True


def validate(path: Path) -> int:
    try:
        documents = list(yaml.safe_load_all(path.read_text(encoding="utf-8")))
    except yaml.YAMLError as error:
        blocked(f"yaml_parse path={path.relative_to(ROOT)}: {error}")
    if len(documents) != 1 or not isinstance(documents[0], dict):
        blocked(f"single_document_required path={path.relative_to(ROOT)} documents={len(documents)}")

    workflow = documents[0]
    jobs = workflow.get("jobs")
    if not isinstance(jobs, dict) or not jobs:
        blocked(f"jobs_mapping_required path={path.relative_to(ROOT)}")

    checked = 0
    defaults = workflow.get("defaults", {})
    for job_name, job in jobs.items():
        if not isinstance(job, dict):
            blocked(f"job_mapping_required path={path.relative_to(ROOT)} job={job_name}")
        job_defaults = job.get("defaults", defaults)
        steps = job.get("steps", [])
        if not isinstance(steps, list):
            blocked(f"steps_list_required path={path.relative_to(ROOT)} job={job_name}")
        for index, step in enumerate(steps):
            if not isinstance(step, dict):
                blocked(f"step_mapping_required path={path.relative_to(ROOT)} job={job_name} step={index}")
            checked += int(validate_bash(path, str(job_name), index, step, job_defaults))
    return checked


def main() -> int:
    files = sorted([*WORKFLOWS.glob("*.yml"), *WORKFLOWS.glob("*.yaml")])
    if not files:
        blocked("workflow_files_missing")
    run_steps = sum(validate(path) for path in files)
    print(f"RAFCODEPHI_WORKFLOW_SYNTAX=PASS workflows={len(files)} run_steps={run_steps}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
